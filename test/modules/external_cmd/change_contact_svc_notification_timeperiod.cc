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

#include <QCoreApplication>
#include <QDebug>
#include <exception>
#include "test/unittest.hh"
#include "logging/engine.hh"
#include "error.hh"
#include "commands.hh"
#include "globals.hh"

using namespace com::centreon::engine;

/**
 *  Run change_contact_svc_notification_timeperiod test.
 */
static void check_change_contact_svc_notification_timeperiod() {
  init_object_skiplists();

  contact* cntct = add_contact("name", NULL, NULL, NULL, NULL, NULL, NULL, 0,
                               0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
  if (!cntct)
    throw (engine_error() << "create contact failed.");

  timeperiod* tperiod = add_timeperiod("tperiod", "alias");
  if (!tperiod)
    throw (engine_error() << "create timeperiod failed.");

  cntct->service_notification_period_ptr = NULL;
  char const* cmd("[1317196300] CHANGE_CONTACT_SVC_NOTIFICATION_TIMEPERIOD;name;tperiod");
  process_external_command(cmd);

  if (cntct->service_notification_period_ptr != tperiod)
    throw (engine_error() << "change_contact_svc_notification_timeperiod failed.");

  delete[] cntct->name;
  delete[] cntct->alias;
  delete[] cntct->service_notification_period;
  delete cntct;

  delete[] tperiod->name;
  delete[] tperiod->alias;
  delete tperiod;

  free_object_skiplists();
}

/**
 *  Check processing of change_contact_svc_notification_timeperiod works.
 */
int main_test() {
  logging::engine& engine = logging::engine::instance();
  check_change_contact_svc_notification_timeperiod();
  engine.cleanup();
  return (0);
}

/**
 *  Init unit test.
 */
int main(int argc, char** argv) {
  QCoreApplication app(argc, argv);
  unittest utest(&main_test);
  QObject::connect(&utest, SIGNAL(finished()), &app, SLOT(quit()));
  utest.start();
  app.exec();
  return (utest.ret());
}