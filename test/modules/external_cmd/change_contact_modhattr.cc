/*
** Copyright 2011 Merethis
**
** This file is part of Centreon Engine.
**
** Centreon Engine is free software: you can redistribute it and/or
** modify it under the terms of the GNU General Public License version 2
** as published by the Free Software Foundation.
**
** Centreon Engine is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
** General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with Centreon Engine. If not, see
** <http://www.gnu.org/licenses/>.
*/

#include <QDebug>
#include <exception>
#include "error.hh"
#include "commands.hh"
#include "globals.hh"
#include "error.hh"

/**
 *  Run change_contact_modhattr test.
 */
static void check_change_contact_modhattr() {
  init_object_skiplists();

  contact* cntct = add_contact("name", NULL, NULL, NULL, NULL, NULL, NULL, 0,
                               0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
  if (!cntct)
    throw (engine_error() << "create contact failed.");

  cntct->modified_host_attributes = 0;
  char const* cmd("[1317196300] CHANGE_CONTACT_MODHATTR;name;42");
  process_external_command(cmd);

  if (cntct->modified_host_attributes != 42)
    throw (engine_error() << "change_contact_modhattr failed.");

  delete[] cntct->name;
  delete[] cntct->alias;
  delete cntct;

  free_object_skiplists();
}

/**
 *  Check processing of change_contact_modhattr works.
 */
int main(void) {
  try {
    check_change_contact_modhattr();
  }
  catch (std::exception const& e) {
    qDebug() << "error: " << e.what();
    return (1);
  }
  return (0);
}
