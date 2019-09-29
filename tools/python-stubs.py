import re


def stub_api_python_prototypes(stub_file):
    prototype_pattern = re.compile(r"""\[source,python\]
----
# prototype
(?:(?P<return>\w+) = )?weechat\.(?P<function>\w+)\((?P<args>[\w\s,]*)\)""")

    args_pattern = re.compile(r"\s*,\s*")

    with open("../doc/en/weechat_plugin_api.en.adoc") as api_doc_file:
        api_doc = api_doc_file.read()

        for m in prototype_pattern.finditer(api_doc):
            m_return = m["return"]
            m_function = m["function"]
            m_args = args_pattern.split(m["args"])

            stub_file.write(f"""def {m_function}({', '.join(m_args)}):
    \"""`{m_function} in WeeChat plugin API reference <https://weechat.org/files/doc/devel/weechat_plugin_api.en.html#_{m_function}>`_\"""
    ...
""")


def stub_scripting_constants(stub_file):
    constant_pattern = re.compile(r"  (?P<constant>WEECHAT_[A-Z0-9_]+)(?: \+)?")

    with open("../doc/en/weechat_scripting.en.adoc") as scripting_file:
        scripting = scripting_file.read()

        for m in constant_pattern.finditer(scripting):
            m_constant = m["constant"]

            stub_file.write(f"{m_constant} = ...\n")


with open("weechat.pyi", "w") as stub_file:
    # constants
    stub_scripting_constants(stub_file)
    stub_file.write("\n")

    # functions
    stub_api_python_prototypes(stub_file)