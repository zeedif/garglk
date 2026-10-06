// Copyright (C) 2006-2009 by Tor Andersson.
// Copyright (C) 2010 by Ben Cressey.
//
// This file is part of Gargoyle.
//
// Gargoyle is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// Gargoyle is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Gargoyle; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

#include <cctype>
#include <cstdlib>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>

#include <unistd.h>

#include "garglk.h"
#include "launcher.h"

#include "kindle.h"

void garglk::winmsg(const std::string &msg)
{
    kindle::dialog(nullptr, msg, kindle::tr("Close"));
}

// The interpreter replaces the launcher, which leaves the memory of the
// device to the game; the game list starts the launcher again.
bool garglk::winterp(const std::string &exe, const std::vector<std::string> &flags, const std::string &game)
{
    auto path = winappdir().value_or(".") + "/" + exe;
    std::vector<char *> argv = {path.data()};

    for (const auto &flag : flags) {
        argv.push_back(const_cast<char *>(flag.c_str()));
    }
    argv.push_back(const_cast<char *>(game.c_str()));
    argv.push_back(nullptr);

    execv(path.c_str(), argv.data());
    winmsg("Could not start interpreter " + path);
    return false;
}

// The platform named on the icon of a game in the game list.
static std::optional<std::string> platform(const std::string &path)
{
    static const std::unordered_map<garglk::Format, const char *> names = {
        {garglk::Format::Adrift, "ADRIFT"},
        {garglk::Format::Adrift5, "ADRIFT"},
        {garglk::Format::AdvSys, "ADVSYS"},
        {garglk::Format::AGT, "AGT"},
        {garglk::Format::Alan2, "ALAN"},
        {garglk::Format::Alan3, "ALAN"},
        {garglk::Format::Glulx, "GLULX"},
        {garglk::Format::Hugo, "HUGO"},
        {garglk::Format::JACL, "JACL"},
        {garglk::Format::Level9, "LEVEL 9"},
        {garglk::Format::Magnetic, "MAGNETIC"},
        {garglk::Format::Plus, "PLUS"},
        {garglk::Format::Scott, "SCOTT"},
        {garglk::Format::TADS, "TADS"},
        {garglk::Format::Taylor, "TAYLOR"},
        {garglk::Format::ZCode, "Z-CODE"},
    };
    auto format = garglk::identify(path);

    if (!format.has_value()) {
        return std::nullopt;
    }

    // Scott Adams games are only told by their extension, which other data
    // files share, so they must also look like one: text starting with a number.
    if (*format == garglk::Format::Scott) {
        std::ifstream file(path);
        char first = '\0';
        if (!(file >> first) || !(std::isdigit(static_cast<unsigned char>(first)) || first == '-')) {
            return std::nullopt;
        }
    }

    return names.at(*format);
}

int main(int argc, char **argv)
{
    gtk_init(&argc, &argv);
    garglk::theme::init();
    gli_read_config(argc, argv);

    if (argc > 1) {
        return garglk::rungame(argv[1]) ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    // rungame only returns when the game could not be started.
    const char *games = std::getenv("GAMES");
    while (auto game = kindle::browse(kindle::Browse::Game, games != nullptr ? games : g_get_home_dir(), "", std::nullopt, platform)) {
        garglk::rungame(*game);
    }

    return EXIT_SUCCESS;
}
