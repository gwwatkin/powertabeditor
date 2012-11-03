/*
  * Copyright (C) 2011 Cameron White
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

#include <catch.hpp>
#include <boost/foreach.hpp>

#include <actions/addirregulargrouping.h>
#include <powertabdocument/position.h>

TEST_CASE("Actions/AddIrregularGrouping", "")
{
    std::vector<Position*> positions;

    Position pos1(0, 1, 0), pos2(1, 1, 0), pos3(2, 1, 0), pos4(3, 1, 0);
    positions.push_back(&pos1);
    positions.push_back(&pos2);
    positions.push_back(&pos3);
    positions.push_back(&pos4);

    AddIrregularGrouping action(positions, 3, 2);

    action.redo();
    BOOST_FOREACH(const Position* pos, positions)
    {
        REQUIRE(pos->HasIrregularGroupingTiming());
    }

    REQUIRE(pos1.IsIrregularGroupingStart());
    REQUIRE(pos2.IsIrregularGroupingMiddle());
    REQUIRE(pos3.IsIrregularGroupingMiddle());
    REQUIRE(pos4.IsIrregularGroupingEnd());

    action.undo();
    BOOST_FOREACH(const Position* pos, positions)
    {
        REQUIRE(!pos->HasIrregularGroupingTiming());
        REQUIRE(!pos->IsIrregularGroupingStart());
        REQUIRE(!pos->IsIrregularGroupingMiddle());
        REQUIRE(!pos->IsIrregularGroupingEnd());
    }
}
