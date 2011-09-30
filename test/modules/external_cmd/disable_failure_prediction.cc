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
 *  Run disable_failure_prediction test.
 */
static void check_disable_failure_prediction() {
  enable_failure_prediction = true;
  char const* cmd("[1317196300] DISABLE_FAILURE_PREDICTION");
  process_external_command(cmd);

  if (enable_failure_prediction)
    throw (engine_error() << "disable_failure_prediction failed.");
}

/**
 *  Check processing of disable_failure_prediction works.
 */
int main(void) {
  try {
    check_disable_failure_prediction();
  }
  catch (std::exception const& e) {
    qDebug() << "error: " << e.what();
    return (1);
  }
  return (0);
}
