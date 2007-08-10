#
# Copyright (c) 2003-2007 by FlashCode <flashcode@flashtux.org>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# fete.pl (c) December 2003 by FlashCode <flashcode@flashtux.org>
# Updated on  2007-08-10    by FlashCode <flashcode@flashtux.org>
#
# Manages french calendat feasts with "/fete" command
# Syntax: /fete            - display today's and tomorrow's feast
#         /fete firstname  - search for name in calendar
#

use locale;

my $version = "0.7";

weechat::register ("Fete", $version, "", "Gestion des fêtes du calendrier français", "UTF-8");
weechat::print ("Script 'Fete' $version chargé");
weechat::add_command_handler ("fete", fete);

@noms_jours = qw(Dimanche Lundi Mardi Mercredi Jeudi Vendredi Samedi);
@noms_mois = qw(Janvier Février Mars Avril Mai Juin Juillet Août Septembre Octobre Novembre Décembre);
@fetes = (
    # janvier
    [ '!Marie - JOUR de l\'AN', '&Basile', '!Geneviève', '&Odilon', '&Edouard',
      '&Mélaine', '&Raymond', '&Lucien', '!Alix', '&Guillaume', '&Paulin',
      '!Tatiana', '!Yvette', '!Nina', '&Rémi', '&Marcel', '!Roseline',
      '!Prisca', '&Marius', '&Sébastien', '!Agnès', '&Vincent', '&Barnard',
      '&François de Sales', '-Conversion de St Paul', '!Paule', '!Angèle',
      '&Thomas d\'Aquin', '&Gildas', '!Martine', '!Marcelle' ],
    # février
    [ '!Ella', '-Présentation', '&Blaise', '!Véronique', '!Agathe',
      '&Gaston', '!Eugénie', '!Jacqueline', '!Apolline', '&Arnaud',
      '-Notre-Dame de Lourdes', '&Félix', '!Béatrice', '&Valentin', '&Claude',
      '!Julienne', '&Alexis', '!Bernadette', '&Gabin', '!Aimée',
      '&Pierre Damien', '!Isabelle', '&Lazare', '&Modeste', '&Roméo', '&Nestor',
      '!Honorine', '&Romain', '&Auguste' ],
    # mars
    [ '&Aubin', '&Charles le Bon', '&Guénolé', '&Casimir', '&Olive', '&Colette',
      '!Félicité', '&Jean de Dieu', '!Françoise', '&Vivien', '!Rosine',
      '!Justine', '&Rodrigue', '!Mathilde', '!Louise de Marillac', '!Bénédicte',
      '&Patrice', '&Cyrille', '&Joseph', '&Herbert', '!Clémence', '!Léa',
      '&Victorien', '!Catherine de Suède', '-Annonciation', '!Larissa',
      '&Habib', '&Gontran', '!Gwladys', '&Amédée', '&Benjamin' ],
    # avril
    [ '&Hugues', '!Sandrine', '&Richard', '&Isodore', '!Irène', '&Marcellin',
      '&Jean-Baptiste de la Salle', '!Julie', '&Gautier', '&Fulbert',
      '&Stanislas', '&Jules', '!Ida', '&Maxime', '&Paterne',
      '&Benoît-Joseph Labre', '&Anicet', '&Parfait', '!Emma', '!Odette',
      '&Anselme', '&Alexandre', '&Georges', '&Fidèle', '&Marc', '!Alida',
      '!Zita', '!Valérie', '!Catherine de Sienne', '&Robert' ],
    # mai
    [ '&Jérémie - FETE du TRAVAIL', '&Boris', '&Philippe / Jacques', '&Sylvain',
      '!Judith', '!Prudence', '!Gisèle', '&Désiré - ANNIVERSAIRE 1945',
      '&Pacôme', '!Solange', '!Estelle', '&Achille', '!Rolande', '&Mathias',
      '!Denise', '&Honoré', '&Pascal', '&Eric', '&Yves', '&Bernardin',
      '&Constantin', '&Emile', '&Didier', '&Donatien', '!Sophie', '&Bérenger',
      '&Augustin', '&Germain', '&Aymar', '&Ferdinand', '-Visitation' ],
    # juin
    [ '&Justin', '!Blandine', '&Kévin', '!Clotilde', '&Igor', '&Norbert',
      '&Gilbert', '&Médard', '!Diane', '&Landry', '&Barnabé', '&Guy',
      '&Antoine de Padoue', '&Elisée', '!Germaine', '&Jean-François Régis',
      '&Hervé', '&Léonce', '&Romuald', '&Silvère', '&Rodolphe', '&Alban',
      '!Audrey', '&Jean-Baptiste', '&Salomon', '&Anthelme', '&Fernand',
      '&Irénée', '&Pierre / Paul', '&Martial' ],
    # juillet
    [ '&Thierry', '&Martinien', '&Thomas', '&Florent', '&Antoine', '!Mariette',
      '&Raoul', '&Thibaut', '!Amandine', '&Ulrich', '&Benoît', '&Olivier',
      '&Henri / Joël', '!Camille - FETE NATIONALE', '&Donald',
      '-N.D. du Mont Carmel', '!Charlotte', '&Frédéric', '&Arsène', '!Marina',
      '&Victor', '!Marie-Madeleine', '!Brigitte', '!Christine', '&Jacques',
      '&Anne', '!Nathalie', '&Samson', '!Marthe', '!Juliette',
      '&Ignace de Loyola' ],
    # août
    [ '&Alphonse', '&Julien', '!Lydie', '&Jean-Marie Vianney', '&Abel',
      '-Transfiguration', '&Gaëtan', '&Dominique', '&Amour', '&Laurent',
      '!Claire', '!Clarisse', '&Hippolyte', '&Evrard',
      '!Marie - ASSOMPTION', '&Armel', '&Hyacinthe', '!Hélène', '&Jean Eudes',
      '&Bernard', '&Christophe', '&Fabrice', '!Rose de Lima', '&Barthélémy',
      '&Louis', '!Natacha', '!Monique', '&Augustin', '!Sabine', '&Fiacre',
      '&Aristide' ],
    # septembre
    [ '&Gilles', '!Ingrid', '&Grégoire', '!Rosalie', '!Raïssa', '&Bertrand',
      '!Reine', '-Nativité de Marie', '&Alain', '!Inès', '&Adelphe',
      '&Apollinaire', '&Aimé', '-La Ste Croix', '&Roland', '!Edith', '&Renaud',
      '!Nadège', '!Emilie', '&Davy', '&Matthieu', '&Maurice', '&Constant',
      '!Thècle', '&Hermann', '&Côme / Damien', '&Vincent de Paul', '&Venceslas',
      '&Michel / Gabriel', '&Jérôme' ],
    # octobre
    [ '!Thérèse de l\'Enfant Jésus', '&Léger', '&Gérard', '&François d\'Assise',
      '!Fleur', '&Bruno', '&Serge', '!Pélagie', '&Denis', '&Ghislain', '&Firmin',
      '&Wilfried', '&Géraud', '&Juste', '!Thérèse d\'Avila', '!Edwige',
      '&Baudouin', '&Luc', '&René', '!Adeline', '!Céline', '!Elodie',
      '&Jean de Capistran', '&Florentin', '&Crépin', '&Dimitri', '!Emeline',
      '&Simon / Jude', '&Narcisse', '!Bienvenue', '&Quentin' ],
    # novembre
    [ '&Harold - TOUSSAINT', '-Défunts', '&Hubert', '&Charles', '!Sylvie',
      '!Bertille', '!Carine', '&Geoffroy', '&Théodore', '&Léon',
      '&Martin - ARMISTICE 1918', '&Christian', '&Brice', '&Sidoine', '&Albert',
      '!Marguerite', '!Elisabeth', '!Aude', '&Tanguy', '&Edmond',
      '-Présentation de Marie', '!Cécile', '&Clément', '!Flora', '!Catherine',
      '!Delphine', '&Séverin', '&Jacques de la Marche', '&Saturnin', '&André' ],
    # décembre
    [ '!Florence', '!Viviane', '&Xavier', '!Barbara', '&Gérald', '&Nicolas',
      '&Ambroise', '-Immaculée Conception', '&Pierre Fourier', '&Romaric',
      '&Daniel', '!Jeanne de Chantal', '!Lucie', '!Odile', '!Ninon', '!Alice',
      '&Gaël', '&Gatien', '&Urbain', '&Abraham', '&Pierre Canisius',
      '!Françoise-Xavier', '&Armand', '!Adèle', '&Emmanuel - NOEL', '&Etienne',
      '&Jean', '-Sts Innocents', '&David', '&Roger', '&Sylvestre' ],
);

sub fete_jour
{
    my ($sec, $min, $heure, $mjour, $mois, $annee, $sjour, $ajour, $est_dst) = localtime ($_[0]);
    my $fete = $fetes[$mois][$mjour-1];
    $fete =~ s/^!/Ste /;
    $fete =~ s/^&/St /;
    $fete =~ s/^-//;
    $fete;
}

sub fete
{
    if ($#_ == 1)
    {
        my @params = split " ", @_[1];
        for $arg (@params)
        {
            for (my $mois = 0; $mois <= $#fetes; $mois++)
            {
                for (my $jour = 0; $jour < 31; $jour++)
                {
                    if (uc ($fetes[$mois][$jour]) =~ /\U$arg/)
                    {
                        weechat::print (($jour + 1)." ".$noms_mois[$mois].": ".substr ($fetes[$mois][$jour], 1));
                    }
                }
            }
        }
    }
    else
    {
        my $time_now = time;
        my ($fete1, $fete2) = (fete_jour ($time_now), fete_jour ($time_now + (3600 * 24)));
        my ($mjour, $mois, $sjour) = (localtime ($time_now))[3, 4, 6];
        weechat::print_infobar (0, "$fete1 (demain: $fete2)");
    }
    return weechat::PLUGIN_RC_OK;
}

fete ();
