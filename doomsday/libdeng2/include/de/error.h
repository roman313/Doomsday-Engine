/*
 * The Doomsday Engine Project -- libdeng2
 *
 * Copyright (c) 2009-2013 Jaakko Keränen <jaakko.keranen@iki.fi>
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

#ifndef LIBDENG2_ERROR_H
#define LIBDENG2_ERROR_H

#include "libdeng2.h"

#include <QString>
#include <string>
#include <stdexcept>

/**
 * @defgroup errors Exceptions
 *
 * These are exceptions thrown by libdeng2 when a fatal error occurs.
 */

namespace de {

/**
 * Base class for error exceptions thrown by libdeng2. @ingroup errors
 */
class DENG2_PUBLIC Error : public std::runtime_error
{
public:
    Error(QString const &where, QString const &message);
    ~Error() throw();

    QString name() const;
    virtual QString asText() const;

protected:
    void setName(QString const &name);

private:
    std::string _name;
};

} // namespace de
    
/**
 * Macro for defining an exception class that belongs to a parent group of
 * exceptions.  This should be used so that whoever uses the class
 * that throws an exception is able to choose the level of generality
 * of the caught errors.
 */
#define DENG2_SUB_ERROR(Parent, Name) \
    class Name : public Parent { \
    public: \
        Name(QString const &message) \
            : Parent("-", message) { Parent::setName(#Name); } \
        Name(QString const &where, QString const &message) \
            : Parent(where, message) { Parent::setName(#Name); } \
        virtual void raise() const { throw *this; } \
    } /**< @note One must put a semicolon after the macro invocation. */

/**
 * Define a top-level exception class.
 * @note One must put a semicolon after the macro invocation.
 */
#define DENG2_ERROR(Name) DENG2_SUB_ERROR(de::Error, Name)

#endif // LIBDENG2_ERROR_H
