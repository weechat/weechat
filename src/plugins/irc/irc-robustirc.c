#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <stdbool.h>
#include <errno.h>

#include <curl/curl.h>
#include <yajl/yajl_gen.h>
#include <yajl/yajl_parse.h>
#include <yajl/yajl_tree.h>

#include "../weechat-plugin.h"
#include "../../core/wee-hook.h"
#include "irc.h"
#include "irc-raw.h"
#include "irc-buffer.h"
#include "irc-config.h"
#include "irc-input.h"
#include "irc-server.h"

// TODO: resolve the _srv record, keep an updateable list of servers around, see the go implementation

// Using a single, shared, multi handle leads to all connections using a shared
// DNS cache, see http://curl.haxx.se/libcurl/c/curl_multi_add_handle.html.
static CURLM *curl_handle;

struct t_body_buffer
{
	char *body;
	size_t size;
};

struct t_robustirc_request
{
	enum {
		RT_CREATESESSION = 0,
		RT_DELETESESSION = 1,
		RT_POSTMESSAGE = 2,
		RT_GETMESSAGES = 3,
	} type;

	struct t_irc_server *server;
	struct t_body_buffer *body;
	struct t_hook *hook_connect;
};

static bool
create_session_done(struct t_robustirc_request *request, CURL *curl)
{
	yajl_val root, sessionid, sessionauth;
	char errmsg[1024];
	const char *ip_address;
	long http_code;

	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
	weechat_log_printf("return code %d", http_code);
	if (http_code != 200) {
		// for errors: (void)(HOOK_CONNECT(hook_connect, callback))(hook_connect->callback_data, WEECHAT_HOOK_CONNECT_MEMORY_ERROR, 0, -1, NULL, NULL);
	}

	root = yajl_tree_parse((const char*)request->body->body, errmsg, sizeof(errmsg));
	if (root == NULL) {
		// TODO: handle error
		return false;
	}

	if (!(sessionid = yajl_tree_get(root, (const char*[]){ "Sessionid", NULL }, yajl_t_string)))
	{
		weechat_log_printf("sessionid not found o_O");
		yajl_tree_free(root);
		return false;
	}

	if (!(sessionauth = yajl_tree_get(root, (const char*[]){ "Sessionauth", NULL }, yajl_t_string)))
	{
		weechat_log_printf("sessionauth not found o_O");
		yajl_tree_free(root);
		return false;
	}

	weechat_log_printf("id = %s", YAJL_GET_STRING(sessionid));
	weechat_log_printf("auth = %s", YAJL_GET_STRING(sessionauth));

	curl_easy_getinfo (curl, CURLINFO_PRIMARY_IP, &ip_address);
	(void)(HOOK_CONNECT(request->hook_connect, callback))
		(request->hook_connect->callback_data,
		 WEECHAT_HOOK_CONNECT_OK,
		 YAJL_GET_STRING(sessionid),
		 YAJL_GET_STRING(sessionauth),
		 NULL,
		 ip_address);
	weechat_unhook (request->hook_connect);
	yajl_tree_free (root);
	return true;
}

struct gm_parse_struct
{
	yajl_handle hand;
	struct t_irc_server *server;
	char *last_key;
	char *data;
};

static size_t
gm_write_func (void *ptr, size_t size, size_t nmemb, void *userdata)
{
	//fprintf(stderr, "gm feeding to parser: %.*s", size*nmemb, ptr);
	struct gm_parse_struct *ps = userdata;
	// TODO: is this multiplication safe?
	if (yajl_parse(ps->hand, ptr, size * nmemb) != yajl_status_ok) {
		fprintf(stderr, "gm parse error\n");
		// TODO: what to return?
	}
	return size * nmemb;
}

static int gm_json_map_key(void *ctx, const unsigned char *val, size_t len)
{
	struct gm_parse_struct *ps = ctx;

	free(ps->last_key);
    ps->last_key = calloc(1, (len + 1) * sizeof(char));
    memcpy(ps->last_key, val, len);

	//fprintf(stderr, "map_key: %.*s\n", len, val);
	return 1;
}

static int gm_json_string(void *ctx, const unsigned char *val, size_t len)
{
	struct gm_parse_struct *ps = ctx;
	// TODO: also check type
	if (strcasecmp(ps->last_key, "data") == 0) {
		//fprintf(stderr, "string: %.*s\n", len, val);
		free(ps->data);
		ps->data = calloc(1, (len+3) * sizeof(char));
		memcpy(ps->data, val, len);
		ps->data[len] = '\r';
		ps->data[len+1] = '\n';
	}
	return 1;
}

static int gm_json_end_map(void *ctx)
{
	struct gm_parse_struct *ps = ctx;
	// TODO: track depth
	if (ps->data != NULL) {
		irc_server_msgq_add_buffer (ps->server, ps->data);
		irc_server_msgq_flush();
		//free(ps->data);
		ps->data = NULL;
	}
	return 1;
}

static yajl_callbacks gm_callbacks = {
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	gm_json_string,
	NULL,
	gm_json_map_key,
	gm_json_end_map,
	NULL,
	NULL
};



static void
get_messages(struct t_irc_server *server)
{

	CURL *curl = curl_easy_init();
	if (!curl) {
		printf(stderr, "multi not initialized\n");
		exit(1);
	}
	struct gm_parse_struct *ps = calloc(1, sizeof(struct gm_parse_struct));
	yajl_gen hand = yajl_alloc(&gm_callbacks, NULL, ps);
	yajl_config(hand, yajl_allow_multiple_values, 1);
	ps->hand = hand;
	ps->server = server;
	char *url;
	// TODO: update robustsession docs to contain 0.0
	// TODO: clarify in robustsession docs whether clientid MUST be different between each message?
	asprintf(&url, "https://localhost:13001/robustirc/v1/%s/messages?lastseen=0.0", server->robustirc_sessionid);
		weechat_printf(server->buffer, "GETting *%s*", url);
	curl_easy_setopt(curl, CURLOPT_URL, url);
	struct curl_slist *headers = NULL;
	headers = curl_slist_append(headers, "Accept: application/json");
	char *auth;
	asprintf(&auth, "X-Session-Auth: %s", server->robustirc_sessionauth);
	headers = curl_slist_append(headers, auth);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, gm_write_func);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, ps);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	// TODO: hook up the CURLOPT_WRITEFUNCTION to a JSON parser and then feed the result into irc_server_recv_cb
	// TODO: check if result is CURLE_OK

	/* Make libcurl immediately start handling the request. */
	curl_multi_add_handle(curl_handle, curl);
	int running;
	curl_multi_socket_action(curl_handle, CURL_SOCKET_TIMEOUT, 0, &running);
}

static void
check_multi_info(void)
{
	CURLMsg *message;
	struct t_robustirc_request *request;
	int pending;

	while ((message = curl_multi_info_read(curl_handle, &pending)))
	{
		if (message->msg != CURLMSG_DONE)
			continue;
		weechat_log_printf("handle done!");

		long http_code;
		curl_easy_getinfo(message->easy_handle, CURLINFO_RESPONSE_CODE, &http_code);
		weechat_log_printf("return code %d", http_code);
		if (http_code == 200) {
		}
		// TODO: trigger the corresponding hook with the appropriate status
		// for errors: (void)(HOOK_CONNECT(hook_connect, callback))(hook_connect->callback_data, WEECHAT_HOOK_CONNECT_MEMORY_ERROR, 0, -1, NULL, NULL);

		curl_easy_getinfo(message->easy_handle, CURLINFO_PRIVATE, &request);
		switch (request->type)
		{
			case RT_CREATESESSION:
				if (create_session_done(request, message->easy_handle))
					get_messages(request->server);
				break;
			case RT_POSTMESSAGE:
				// TODO: check the return code and retry if it’s not 200.
				weechat_log_printf("body for post = %s", request->body->body);
				break;
			default:
				assert(false);
		}


		curl_multi_remove_handle(curl_handle, message->easy_handle);
		curl_easy_cleanup(message->easy_handle);
		free(request->body->body);
		free(request->body);
		free(request);
	}
}

/* WeeChat callback which notifies libcurl about events on file descriptor |fd|. */
static int
socket_recv_cb (void *data, int fd)
{
	int running;
	// TODO: check error
	curl_multi_socket_action(curl_handle, fd, 0, &running);
	check_multi_info();
	return WEECHAT_RC_OK;
}

/* WeeChat callback which notifies libcurl about a timeout. */
static int
timeout_cb (void *data, int remaining_calls)
{
	int running;
	// TODO: check error
	curl_multi_socket_action(curl_handle, CURL_SOCKET_TIMEOUT, 0, &running);
	check_multi_info();
	return WEECHAT_RC_OK;
}

/* libcurl callback which sets up a WeeChat hook to watch for events on socket |s|. */
static int
socket_callback(CURL *easy, curl_socket_t s, int what, void *userp, void *socketp)
{
	if (what == CURL_POLL_NONE)
		return 0;

	if (what == CURL_POLL_REMOVE)
		// TODO: save the hook so that we can handle CURL_POLL_REMOVE. see http://curl.haxx.se/libcurl/c/multi-uv.html
		return 0;

            weechat_hook_fd (s,
                                               (what == CURL_POLL_IN || what == CURL_POLL_INOUT),
											   (what == CURL_POLL_OUT || what == CURL_POLL_INOUT),
											   0,
                                               &socket_recv_cb,
                                               NULL);
	return 0;
}

/* libcurl callback to adjust the timeout of our WeeChat timer. */
static int
start_timeout(CURLM *multi,    /* multi handle */
                    long timeout_ms, /* see above */
                    void *userp)    /* private callback pointer */
{
	// TODO: handle timeout_ms==-1. what does it mean?
	// TODO: how does weechat_hook_timer handle a value of timeout_ms==0?
	// TODO: overwrite the same hook and free the old hook if existing
	// TODO: should this be a repeated timer or is max_calls=1 correct?
	weechat_hook_timer(timeout_ms, 0, 1, timeout_cb, NULL);
	return 0;
}

static size_t
write_func(void *contents, size_t size, size_t nmemb, void *userp)
{
	// TODO: document why this is safe
	size_t realsize = size * nmemb;
	struct t_robustirc_request *request = userp;
	struct t_body_buffer *body_buffer = request->body;

	body_buffer->body = realloc(body_buffer->body, body_buffer->size + realsize + 1);
	if (body_buffer->body == NULL) {
		// TODO: handle error
		return 0;
	}

	memcpy(&(body_buffer->body[body_buffer->size]), contents, realsize);
	// TODO: document why this is safe
	body_buffer->size += realsize;
	body_buffer->body[body_buffer->size] = 0;

	return realsize;
}

int
irc_robustirc_init (void)
{
	if (curl_global_init(CURL_GLOBAL_DEFAULT) != 0)
		return WEECHAT_RC_ERROR;

	if (!(curl_handle = curl_multi_init()))
		return WEECHAT_RC_ERROR;

	curl_multi_setopt(curl_handle, CURLMOPT_SOCKETFUNCTION, socket_callback);
	curl_multi_setopt(curl_handle, CURLMOPT_TIMERFUNCTION, start_timeout);

	return WEECHAT_RC_OK;
}

int
irc_robustirc_send (struct t_irc_server *server, const char *buffer, int size_buf)
{
	char *url = NULL;
	yajl_gen gen = NULL;
	CURL *curl = NULL;
	struct t_robustirc_request *request = NULL;

	if (!(curl = curl_easy_init()))
	{
		weechat_printf (
			server->buffer,
			_("%s%s: sending data to server (robustsession): curl_easy_init() failed"),
			weechat_prefix ("error"), IRC_PLUGIN_NAME);
		goto error;
	}

	if (!(gen = yajl_gen_alloc(NULL)))
	{
		weechat_printf (
			server->buffer,
			_("%s%s: sending data to server (robustsession): yajl_gen_alloc() failed"),
			weechat_prefix ("error"), IRC_PLUGIN_NAME);
		goto error;
	}

	// TODO: yajl error handling
	yajl_gen_map_open(gen);
	yajl_gen_string(gen, "Data", strlen("Data"));
	yajl_gen_string(gen, buffer, size_buf);
	yajl_gen_string(gen, "ClientMessageId", strlen("ClientMessageId"));
	// TODO: better ClientMessageId, hash the message
	yajl_gen_integer(gen, size_buf + rand());
	yajl_gen_map_close(gen);
	const unsigned char *body = NULL;
	size_t len = 0;
	yajl_gen_get_buf(gen, &body, &len);

	if (asprintf(&url, "https://localhost:13001/robustirc/v1/%s/message", server->robustirc_sessionid) == -1)
	{
		weechat_printf (
			server->buffer,
			_("%s%s: sending data to server (robustsession): asprintf(): %s"),
			weechat_prefix ("error"), IRC_PLUGIN_NAME,
			strerror (errno));
		goto error;
	}

	if (!(request = calloc(1, sizeof(*request))))
	{
		weechat_printf (
			server->buffer,
			_("%s%s: sending data to server (robustsession): calloc(): %s"),
			weechat_prefix ("error"), IRC_PLUGIN_NAME,
			strerror (errno));
		goto error;
	}
	request->type = RT_POSTMESSAGE;
	request->body = calloc(1, sizeof(struct t_body_buffer));

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_POST, 1);
	struct curl_slist *headers = NULL;
	headers = curl_slist_append(headers, "Accept: application/json");
	headers = curl_slist_append(headers, "Content-Type: application/json");
	// TODO: free this? when? does curl_easy_cleanup() handle it?
	char *auth;
	asprintf(&auth, "X-Session-Auth: %s", server->robustirc_sessionauth);
	headers = curl_slist_append(headers, auth);
	// TODO: free this? when? does curl_easy_cleanup() handle it?
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_func);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, request);
	curl_easy_setopt(curl, CURLOPT_PRIVATE, request);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);

	/* Make libcurl immediately start handling the request. */
	curl_multi_add_handle(curl_handle, curl);
	int running;
	curl_multi_socket_action(curl_handle, CURL_SOCKET_TIMEOUT, 0, &running);

	// TODO: what to free?
	return size_buf;

	// TODO: get sessionid and sessionauth
	// TODO: return number of bytes sent

error:
	if (curl != NULL)
		curl_easy_cleanup(curl);
	if (gen != NULL)
		yajl_gen_free(gen);
	free(url);
	if (request != NULL)
	{
		free(request->body);
	}
	free(request);
	return -1; // error
}

// TODO: also hook up irc_server_disconnect
// TODO: return a hook. see weechat_hook_connect
void
irc_robustirc_connect (struct t_irc_server *server, const char *address, t_hook_callback_connect_robustirc *callback, void *callback_data)
{
    struct t_hook *new_hook;
    new_hook = weechat_hook_connect_robustirc(address, callback, callback_data);
    if (!new_hook)
        return NULL;

	CURL *curl = curl_easy_init();
	if (!curl) {
		fprintf(stderr, "easy not initialized\n");
		/* TODO: error */
		return NULL;
	}

	struct t_robustirc_request *request = calloc(1, sizeof(struct t_robustirc_request));
	if (!request)
		// TODO: error?
		return;
	request->type = RT_CREATESESSION;
	request->body = calloc(1, sizeof(struct t_body_buffer));
	request->hook_connect = new_hook;
	request->server = server;
	//request->yajl_parser = yajl_alloc(&callbacks, NULL, request);
	// TODO: set this from address
	curl_easy_setopt(curl, CURLOPT_URL, "https://localhost:13001/robustirc/v1/session");
	curl_easy_setopt(curl, CURLOPT_POST, 1);
	// TODO: set this from ssl_verify option
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_func);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, request);
	curl_easy_setopt(curl, CURLOPT_PRIVATE, request);

	// TODO: set ipv6 from ipv6 option
	// TODO: set proxy option



	/* Make libcurl immediately start handling the request. */
	curl_multi_add_handle(curl_handle, curl);
	int running;
	curl_multi_socket_action(curl_handle, CURL_SOCKET_TIMEOUT, 0, &running);

	// TODO: start curl with new_hook as cb

	return new_hook;
}
