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
#include "broker.hh"
#include "nebstructs.hh"

/**
 *  callback used by broker.
 *
 *  @param[in] callback_type The callback type send by the broker.
 *  @param[in] data          The nebstruct external command send by the broker.
 *
 *  @return the last callback_type recve by the callback.
 */
static int broker_callback(int callback_type, void* data) {
  static int last_callback_type = -1;
  int ret = last_callback_type;

  nebstruct_external_command_data* neb_data
    = static_cast<nebstruct_external_command_data*>(data);
  if (callback_type != -1)
    last_callback_type = neb_data->type;
  else
    last_callback_type = -1;
  return (ret);
}

/**
 *  Run read_state_information test.
 */
static void check_read_state_information() {
  config.set_retain_state_information(true);

  // register broker callback to catch event.
  config.set_event_broker_options(BROKER_RETENTION_DATA);
  void* module_id = reinterpret_cast<void*>(0x4242);
  neb_register_callback(NEBCALLBACK_RETENTION_DATA,
                        module_id,
                        0,
                        &broker_callback);

  char const* cmd("[1317196300] READ_STATE_INFORMATION");
  process_external_command(cmd);

  if (broker_callback(-1, NULL) != NEBTYPE_RETENTIONDATA_ENDLOAD)
    throw (engine_error() << "read_state_information failed.");

  // release callback.
  neb_deregister_module_callbacks(module_id);
}

/**
 *  Check processing of read_state_information works.
 */
int main(void) {
  try {
    check_read_state_information();
  }
  catch (std::exception const& e) {
    qDebug() << "error: " << e.what();
    return (1);
  }
  return (0);
}
