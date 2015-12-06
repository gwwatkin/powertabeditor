/*
  * Copyright (C) 2015 Cameron White
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  *
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.
  *
  * You should have received a copy of the GNU General Public License
  * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "settingsmanager.h"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

#include <rapidjson/prettywriter.h>
#include <util/rapidjson_iostreams.h>

using SettingValue = SettingsManager::SettingValue;
using SettingList = SettingsManager::SettingList;
using SettingMap = SettingsManager::SettingMap;

struct IsMap : public boost::static_visitor<bool>
{
    template <typename T>
    bool operator()(const T &) const
    {
        return false;
    }

    bool operator()(const SettingMap &) const
    {
        return true;
    }
};

struct Inserter : public boost::static_visitor<>
{
    Inserter(const std::string &key, SettingValue value)
        : myIndex(0), myNewValue(std::move(value))
    {
        boost::algorithm::split(myComponents, key, boost::is_any_of("/"));
    }

    template <typename T>
    void operator()(T &)
    {
        assert(false);
    }

    void operator()(SettingMap &map)
    {
        SettingValue &value = map[myComponents[myIndex]];

        if (myIndex == myComponents.size() - 1)
            value = myNewValue;
        else
        {
            if (!boost::apply_visitor(IsMap(), value))
                value = SettingMap();

            ++myIndex;
            boost::apply_visitor(*this, value);
        }
    }

private:
    std::vector<std::string> myComponents;
    size_t myIndex;
    SettingValue myNewValue;
};

struct Finder : public boost::static_visitor<boost::optional<SettingValue>>
{
    Finder(const std::string &key)
        : myIndex(0)
    {
        boost::algorithm::split(myComponents, key, boost::is_any_of("/"));
    }

    template <typename T>
    boost::optional<SettingValue> operator()(const T &) const
    {
        return boost::none;
    }

    boost::optional<SettingValue> operator()(const SettingMap &map) const
    {
        auto it = map.find(myComponents[myIndex]);
        if (it == map.end())
            return boost::none;

        const SettingValue &value = it->second;
        if (myIndex == myComponents.size() - 1)
            return value;
        else
        {
            if (!boost::apply_visitor(IsMap(), value))
                return boost::none;

            ++myIndex;
            return boost::apply_visitor(*this, value);
        }
    }

private:
    std::vector<std::string> myComponents;
    mutable size_t myIndex;
};

struct JSONSerializer : public boost::static_visitor<void>
{
    JSONSerializer(std::ostream &os) : myStream(os), myWriter(myStream)
    {
    }

    void operator()(int x)
    {
        myWriter.Int(x);
    }

    void operator()(unsigned int x)
    {
        myWriter.Uint(x);
    }

    void operator()(const std::string &s)
    {
        myWriter.String(s.c_str(), s.length());
    }

    void operator()(const SettingList &list)
    {
        myWriter.StartArray();
        for (auto &&value : list)
            boost::apply_visitor(*this, value);
        myWriter.EndArray();
    }

    void operator()(const SettingMap &map)
    {
        myWriter.StartObject();

        // Output in sorted order.
        std::vector<std::string> keys;
        keys.reserve(map.size());
        for (auto &&pair : map)
            keys.push_back(pair.first);

        std::sort(keys.begin(), keys.end());

        for (auto &&key : keys)
        {
            myWriter.Key(key.c_str());
            boost::apply_visitor(*this, map.at(key));
        }

        myWriter.EndObject();
    }

private:
    Util::RapidJSON::OStreamWrapper myStream;
    rapidjson::PrettyWriter<decltype(myStream)> myWriter;
};

SettingsManager::SettingsManager() : myTree(SettingMap())
{
}

void SettingsManager::set(const std::string &key, const SettingValue &value)
{
    Inserter inserter(key, value);
    boost::apply_visitor(inserter, myTree);
}

boost::optional<SettingValue> SettingsManager::find(
    const std::string &key) const
{
    return boost::apply_visitor(Finder(key), myTree);
}

void SettingsManager::save(std::ostream &os) const
{
    JSONSerializer serializer(os);
    boost::apply_visitor(serializer, myTree);
}
