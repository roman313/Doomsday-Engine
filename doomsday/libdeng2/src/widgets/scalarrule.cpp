/*
 * The Doomsday Engine Project
 *
 * Copyright (c) 2011 Jaakko Keränen <jaakko.keranen@iki.fi>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include "de/ScalarRule"
#include "de/Clock"
#include <QDebug>

namespace de {

ScalarRule::ScalarRule(float initialValue)
    : Rule(initialValue), _animation(initialValue), _targetRule(0)
{}

ScalarRule::~ScalarRule()
{
    independentOf(_targetRule);
}

void ScalarRule::set(float target, TimeDelta transition, TimeDelta delay)
{
    independentOf(_targetRule);
    _targetRule = 0;

    _animation.clock().audienceForPriorityTimeChange += this;

    _animation.setValue(target, transition, delay);
    invalidate();
}

void ScalarRule::set(Rule const &target, TimeDelta transition, TimeDelta delay)
{
    set(target.value(), transition, delay);

    // Keep a reference.
    _targetRule = &target;
    dependsOn(_targetRule);
}

void ScalarRule::setStyle(Animation::Style style)
{
    _animation.setStyle(style);
}

void ScalarRule::setStyle(Animation::Style style, float bounceSpring)
{
    _animation.setStyle(style, bounceSpring);
}

void ScalarRule::shift(float delta)
{
    _animation.shift(delta);
    invalidate();
}

String ScalarRule::description() const
{
    String desc = "Scalar(" + _animation.asText();
    if(_targetRule)
    {
        desc += "; target: " + _targetRule->description();
    }
    return desc + ")";
}

void ScalarRule::update()
{
    // When using a rule for the target, keep it updated.
    if(_targetRule)
    {
        _animation.adjustTarget(_targetRule->value());
    }

    setValue(_animation);
}

void ScalarRule::timeChanged(Clock const &clock)
{
    invalidate();

    if(_animation.done())
    {
        clock.audienceForPriorityTimeChange -= this;
    }
}

} // namespace de
