/*
** Copyright 2011      Merethis
**
** This file is part of Centreon Scheduler.
**
** Centreon Scheduler is free software: you can redistribute it and/or
** modify it under the terms of the GNU General Public License version 2
** as published by the Free Software Foundation.
**
** Centreon Scheduler is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
** General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with Centreon Scheduler. If not, see
** <http://www.gnu.org/licenses/>.
*/

#include <sstream>
#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include "nagios.hh"
#include "logging.hh"
#include "common.hh"
#include "macros.hh"
#include "broker.hh"
#include "nebmods.hh"
#include "utils.hh"

#include "error.hh"
#include "configuration.hh"

extern sched_info scheduling_info;
extern char       *macro_user[MAX_USER_MACROS];

using namespace com::centreon::scheduler;

const float configuration::DEFAULT_SLEEP_TIME                  = 0.5;
const float configuration::DEFAULT_LOW_SERVICE_FLAP_THRESHOLD  = 20.0;
const float configuration::DEFAULT_HIGH_SERVICE_FLAP_THRESHOLD = 30.0;
const float configuration::DEFAULT_LOW_HOST_FLAP_THRESHOLD     = 20.0;
const float configuration::DEFAULT_HIGH_HOST_FLAP_THRESHOLD    = 30.0;

/**************************************
 *                                     *
 *           Public Methods            *
 *                                     *
 **************************************/

/**
 *  Default constructor.
 */
configuration::configuration()
  : _cur_line(0), _command_check_interval_is_seconds(false), _mac(NULL) {

  _mac = get_global_macros();

  _lst_method["resource_file"]                               = &cpp_suck<std::string const&, &configuration::_parse_resource_file>::set_generic;;
  _lst_method["log_file"]                                    = &cpp_suck<std::string const&, &configuration::set_log_file>::set_generic;
  _lst_method["debug_level"]                                 = &cpp_suck<unsigned int, &configuration::set_debug_level>::set_generic;
  _lst_method["debug_verbosity"]                             = &cpp_suck<unsigned int, &configuration::set_debug_verbosity>::set_generic;
  _lst_method["debug_file"]                                  = &cpp_suck<std::string const&, &configuration::set_debug_file>::set_generic;
  _lst_method["max_debug_file_size"]                         = &cpp_suck<unsigned long, &configuration::set_max_debug_file_size>::set_generic;
  _lst_method["command_file"]                                = &cpp_suck<std::string const&, &configuration::set_command_file>::set_generic;
  _lst_method["temp_file"]                                   = &cpp_suck<std::string const&, &configuration::set_temp_file>::set_generic;
  _lst_method["temp_path"]                                   = &cpp_suck<std::string const&, &configuration::set_temp_path>::set_generic;
  _lst_method["check_result_path"]                           = &cpp_suck<std::string const&, &configuration::set_check_result_path>::set_generic;
  _lst_method["max_check_result_file_age"]                   = &cpp_suck<unsigned long, &configuration::set_max_check_result_file_age>::set_generic;
  _lst_method["global_host_event_handler"]                   = &cpp_suck<std::string const&, &configuration::set_global_host_event_handler>::set_generic;
  _lst_method["global_service_event_handler"]                = &cpp_suck<std::string const&, &configuration::set_global_service_event_handler>::set_generic;
  _lst_method["ocsp_command"]                                = &cpp_suck<std::string const&, &configuration::set_ocsp_command>::set_generic;
  _lst_method["ochp_command"]                                = &cpp_suck<std::string const&, &configuration::set_ochp_command>::set_generic;
  _lst_method["admin_email"]                                 = &cpp_suck<std::string const&, &configuration::_set_admin_email>::set_generic;
  _lst_method["admin_pager"]                                 = &cpp_suck<std::string const&, &configuration::_set_admin_pager>::set_generic;
  _lst_method["use_syslog"]                                  = &cpp_suck<bool, &configuration::set_use_syslog>::set_generic;
  _lst_method["log_notifications"]                           = &cpp_suck<bool, &configuration::set_log_notifications>::set_generic;
  _lst_method["log_service_retries"]                         = &cpp_suck<bool, &configuration::set_log_service_retries>::set_generic;
  _lst_method["log_host_retries"]                            = &cpp_suck<bool, &configuration::set_log_host_retries>::set_generic;
  _lst_method["log_event_handlers"]                          = &cpp_suck<bool, &configuration::set_log_event_handlers>::set_generic;
  _lst_method["log_external_commands"]                       = &cpp_suck<bool, &configuration::set_log_external_commands>::set_generic;
  _lst_method["log_passive_checks"]                          = &cpp_suck<bool, &configuration::set_log_passive_checks>::set_generic;
  _lst_method["log_initial_states"]                          = &cpp_suck<bool, &configuration::set_log_initial_states>::set_generic;
  _lst_method["retain_state_information"]                    = &cpp_suck<bool, &configuration::set_retain_state_information>::set_generic;
  _lst_method["retention_update_interval"]                   = &cpp_suck<unsigned int, &configuration::set_retention_update_interval>::set_generic;
  _lst_method["use_retained_program_state"]                  = &cpp_suck<bool, &configuration::set_use_retained_program_state>::set_generic;
  _lst_method["use_retained_scheduling_info"]                = &cpp_suck<bool, &configuration::set_use_retained_scheduling_info>::set_generic;
  _lst_method["retention_scheduling_horizon"]                = &cpp_suck<unsigned int, &configuration::set_retention_scheduling_horizon>::set_generic;
  _lst_method["additional_freshness_latency"]                = &cpp_suck<int, &configuration::set_additional_freshness_latency>::set_generic;
  _lst_method["retained_host_attribute_mask"]                = &cpp_suck<unsigned long, &configuration::set_retained_host_attribute_mask>::set_generic;
  _lst_method["retained_service_attribute_mask"]             = &cpp_suck<std::string const&, &configuration::_set_retained_service_attribute_mask>::set_generic;
  _lst_method["retained_process_host_attribute_mask"]        = &cpp_suck<unsigned long, &configuration::set_retained_process_host_attribute_mask>::set_generic;
  _lst_method["retained_process_service_attribute_mask"]     = &cpp_suck<std::string const&, &configuration::_set_retained_process_service_attribute_mask>::set_generic;
  _lst_method["retained_contact_host_attribute_mask"]        = &cpp_suck<unsigned long, &configuration::set_retained_contact_host_attribute_mask>::set_generic;
  _lst_method["retained_contact_service_attribute_mask"]     = &cpp_suck<unsigned long, &configuration::set_retained_contact_service_attribute_mask>::set_generic;
  _lst_method["obsess_over_services"]                        = &cpp_suck<bool, &configuration::set_obsess_over_services>::set_generic;
  _lst_method["obsess_over_hosts"]                           = &cpp_suck<bool, &configuration::set_obsess_over_hosts>::set_generic;
  _lst_method["translate_passive_host_checks"]               = &cpp_suck<bool, &configuration::set_translate_passive_host_checks>::set_generic;
  _lst_method["passive_host_checks_are_soft"]                = &cpp_suck<bool, &configuration::set_passive_host_checks_are_soft>::set_generic;
  _lst_method["service_check_timeout"]                       = &cpp_suck<unsigned int, &configuration::set_service_check_timeout>::set_generic;
  _lst_method["host_check_timeout"]                          = &cpp_suck<unsigned int, &configuration::set_host_check_timeout>::set_generic;
  _lst_method["event_handler_timeout"]                       = &cpp_suck<unsigned int, &configuration::set_event_handler_timeout>::set_generic;
  _lst_method["notification_timeout"]                        = &cpp_suck<unsigned int, &configuration::set_notification_timeout>::set_generic;
  _lst_method["ocsp_timeout"]                                = &cpp_suck<unsigned int, &configuration::set_ocsp_timeout>::set_generic;
  _lst_method["ochp_timeout"]                                = &cpp_suck<unsigned int, &configuration::set_ochp_timeout>::set_generic;
  _lst_method["use_agressive_host_checking"]                 = &cpp_suck<bool, &configuration::set_use_aggressive_host_checking>::set_generic;
  _lst_method["use_aggressive_host_checking"]                = &cpp_suck<bool, &configuration::set_use_aggressive_host_checking>::set_generic;
  _lst_method["cached_host_check_horizon"]                   = &cpp_suck<unsigned long, &configuration::set_cached_host_check_horizon>::set_generic;
  _lst_method["enable_predictive_host_dependency_checks"]    = &cpp_suck<bool, &configuration::set_enable_predictive_host_dependency_checks>::set_generic;
  _lst_method["cached_service_check_horizon"]                = &cpp_suck<unsigned long, &configuration::set_cached_service_check_horizon>::set_generic;
  _lst_method["enable_predictive_service_dependency_checks"] = &cpp_suck<bool, &configuration::set_enable_predictive_service_dependency_checks>::set_generic;
  _lst_method["soft_state_dependencies"]                     = &cpp_suck<bool, &configuration::set_soft_state_dependencies>::set_generic;
  _lst_method["log_rotation_method"]                         = &cpp_suck<std::string const&, &configuration::set_log_rotation_method>::set_generic;
  _lst_method["log_archive_path"]                            = &cpp_suck<std::string const&, &configuration::set_log_archive_path>::set_generic;
  _lst_method["enable_event_handlers"]                       = &cpp_suck<bool, &configuration::set_enable_event_handlers>::set_generic;
  _lst_method["enable_notifications"]                        = &cpp_suck<bool, &configuration::set_enable_notifications>::set_generic;
  _lst_method["execute_service_checks"]                      = &cpp_suck<bool, &configuration::set_execute_service_checks>::set_generic;
  _lst_method["accept_passive_service_checks"]               = &cpp_suck<bool, &configuration::set_accept_passive_service_checks>::set_generic;
  _lst_method["execute_host_checks"]                         = &cpp_suck<bool, &configuration::set_execute_host_checks>::set_generic;
  _lst_method["accept_passive_host_checks"]                  = &cpp_suck<bool, &configuration::set_accept_passive_host_checks>::set_generic;
  _lst_method["service_inter_check_delay_method"]            = &cpp_suck<std::string const&, &configuration::set_service_inter_check_delay_method>::set_generic;
  _lst_method["max_service_check_spread"]                    = &cpp_suck<unsigned int, &configuration::set_max_service_check_spread>::set_generic;
  _lst_method["host_inter_check_delay_method"]               = &cpp_suck<std::string const&, &configuration::set_host_inter_check_delay_method>::set_generic;
  _lst_method["max_host_check_spread"]                       = &cpp_suck<unsigned int, &configuration::set_max_host_check_spread>::set_generic;
  _lst_method["service_interleave_factor"]                   = &cpp_suck<std::string const&, &configuration::set_service_interleave_factor_method>::set_generic;
  _lst_method["max_concurrent_checks"]                       = &cpp_suck<unsigned int, &configuration::set_max_concurrent_checks>::set_generic;
  _lst_method["check_result_reaper_frequency"]               = &cpp_suck<unsigned int, &configuration::set_check_reaper_interval>::set_generic;
  _lst_method["service_reaper_frequency"]                    = &cpp_suck<unsigned int, &configuration::set_check_reaper_interval>::set_generic;
  _lst_method["max_check_result_reaper_time"]                = &cpp_suck<unsigned int, &configuration::set_max_check_reaper_time>::set_generic;
  _lst_method["sleep_time"]                                  = &cpp_suck<float, &configuration::set_sleep_time>::set_generic;
  _lst_method["interval_length"]                             = &cpp_suck<unsigned int, &configuration::set_interval_length>::set_generic;
  _lst_method["check_external_commands"]                     = &cpp_suck<bool, &configuration::set_check_external_commands>::set_generic;
  _lst_method["command_check_interval"]                      = &cpp_suck<std::string const&, &configuration::set_command_check_interval>::set_generic;
  _lst_method["check_for_orphaned_services"]                 = &cpp_suck<bool, &configuration::set_check_orphaned_services>::set_generic;
  _lst_method["check_for_orphaned_hosts"]                    = &cpp_suck<bool, &configuration::set_check_orphaned_hosts>::set_generic;
  _lst_method["check_service_freshness"]                     = &cpp_suck<bool, &configuration::set_check_service_freshness>::set_generic;
  _lst_method["check_host_freshness"]                        = &cpp_suck<bool, &configuration::set_check_host_freshness>::set_generic;
  _lst_method["service_freshness_check_interval"]            = &cpp_suck<unsigned int, &configuration::set_service_freshness_check_interval>::set_generic;
  _lst_method["host_freshness_check_interval"]               = &cpp_suck<unsigned int, &configuration::set_host_freshness_check_interval>::set_generic;
  _lst_method["auto_reschedule_checks"]                      = &cpp_suck<bool, &configuration::set_auto_reschedule_checks>::set_generic;
  _lst_method["auto_rescheduling_interval"]                  = &cpp_suck<unsigned int, &configuration::set_auto_rescheduling_interval>::set_generic;
  _lst_method["auto_rescheduling_window"]                    = &cpp_suck<unsigned int, &configuration::set_auto_rescheduling_window>::set_generic;
  _lst_method["aggregate_status_updates"]                    = &cpp_suck<std::string const&, &configuration::_set_aggregate_status_updates>::set_generic;
  _lst_method["status_update_interval"]                      = &cpp_suck<unsigned int, &configuration::set_status_update_interval>::set_generic;
  _lst_method["time_change_threshold"]                       = &cpp_suck<unsigned int, &configuration::set_time_change_threshold>::set_generic;
  _lst_method["process_performance_data"]                    = &cpp_suck<bool, &configuration::set_process_performance_data>::set_generic;
  _lst_method["enable_flap_detection"]                       = &cpp_suck<bool, &configuration::set_enable_flap_detection>::set_generic;
  _lst_method["enable_failure_prediction"]                   = &cpp_suck<bool, &configuration::set_enable_failure_prediction>::set_generic;
  _lst_method["low_service_flap_threshold"]                  = &cpp_suck<float, &configuration::set_low_service_flap_threshold>::set_generic;
  _lst_method["high_service_flap_threshold"]                 = &cpp_suck<float, &configuration::set_high_service_flap_threshold>::set_generic;
  _lst_method["low_host_flap_threshold"]                     = &cpp_suck<float, &configuration::set_low_host_flap_threshold>::set_generic;
  _lst_method["high_host_flap_threshold"]                    = &cpp_suck<float, &configuration::set_high_host_flap_threshold>::set_generic;
  _lst_method["date_format"]                                 = &cpp_suck<std::string const&, &configuration::set_date_format>::set_generic;
  _lst_method["use_timezone"]                                = &cpp_suck<std::string const&, &configuration::set_use_timezone>::set_generic;
  _lst_method["p1_file"]                                     = &cpp_suck<std::string const&, &configuration::set_p1_file>::set_generic;
  _lst_method["event_broker_options"]                        = &cpp_suck<std::string const&, &configuration::set_event_broker_options>::set_generic;
  _lst_method["illegal_object_name_chars"]                   = &cpp_suck<std::string const&, &configuration::set_illegal_object_chars>::set_generic;
  _lst_method["illegal_macro_output_chars"]                  = &cpp_suck<std::string const&, &configuration::set_illegal_output_chars>::set_generic;
  _lst_method["broker_module"]                               = &cpp_suck<std::string const&, &configuration::_set_broker_module>::set_generic;
  _lst_method["use_regexp_matching"]                         = &cpp_suck<bool, &configuration::set_use_regexp_matches>::set_generic;
  _lst_method["use_true_regexp_matching"]                    = &cpp_suck<bool, &configuration::set_use_true_regexp_matching>::set_generic;
  _lst_method["use_large_installation_tweaks"]               = &cpp_suck<bool, &configuration::set_use_large_installation_tweaks>::set_generic;
  _lst_method["enable_environment_macros"]                   = &cpp_suck<bool, &configuration::set_enable_environment_macros>::set_generic;
  _lst_method["free_child_process_memory"]                   = &cpp_suck<bool, &configuration::set_free_child_process_memory>::set_generic;
  _lst_method["child_processes_fork_twice"]                  = &cpp_suck<bool, &configuration::set_child_processes_fork_twice>::set_generic;
  _lst_method["enable_embedded_perl"]                        = &cpp_suck<bool, &configuration::set_enable_embedded_perl>::set_generic;
  _lst_method["use_embedded_perl_implicitly"]                = &cpp_suck<bool, &configuration::set_use_embedded_perl_implicitly>::set_generic;
  _lst_method["external_command_buffer_slots"]               = &cpp_suck<int, &configuration::set_external_command_buffer_slots>::set_generic;
  _lst_method["auth_file"]                                   = &cpp_suck<std::string const&, &configuration::_set_auth_file>::set_generic;
  _lst_method["bare_update_check"]                           = &cpp_suck<std::string const&, &configuration::_set_bare_update_check>::set_generic;
  _lst_method["check_for_updates"]                           = &cpp_suck<std::string const&, &configuration::_set_check_for_updates>::set_generic;
  _lst_method["comment_file"]                                = &cpp_suck<std::string const&, &configuration::_set_comment_file>::set_generic;
  _lst_method["xcddefault_comment_file"]                     = &cpp_suck<std::string const&, &configuration::_set_comment_file>::set_generic;
  _lst_method["daemon_dumps_core"]                           = &cpp_suck<std::string const&, &configuration::_set_daemon_dumps_core>::set_generic;
  _lst_method["downtime_file"]                               = &cpp_suck<std::string const&, &configuration::_set_downtime_file>::set_generic;
  _lst_method["xdddefault_downtime_file"]                    = &cpp_suck<std::string const&, &configuration::_set_downtime_file>::set_generic;
  _lst_method["lock_file"]                                   = &cpp_suck<std::string const&, &configuration::_set_lock_file>::set_generic;
  _lst_method["nagios_user"]                                 = &cpp_suck<std::string const&, &configuration::_set_nagios_user>::set_generic;
  _lst_method["nagios_group"]                                = &cpp_suck<std::string const&, &configuration::_set_nagios_group>::set_generic;
  _lst_method["allow_empty_hostgroup_assignment"]            = &cpp_suck<bool, &configuration::set_allow_empty_hostgroup_assignment>::set_generic;

  _lst_method["status_file"]                                 = NULL; // ignore external variables
  _lst_method["perfdata_timeout"]                            = NULL; // ignore external variables
  _lst_method["cfg_file"]                                    = NULL;
  _lst_method["cfg_dir"]                                     = NULL;
  _lst_method["state_retention_file"]                        = NULL;
  _lst_method["object_cache_file"]                           = NULL;
  _lst_method["precached_object_file"]                       = NULL;

  _reset();

  set_accept_passive_host_checks(DEFAULT_ACCEPT_PASSIVE_HOST_CHECKS);
  set_allow_empty_hostgroup_assignment(DEFAULT_ALLOW_EMPTY_HOSTGROUP_ASSIGNMENT);

  _tab_string[temp_path] = DEFAULT_TEMP_PATH;
  _tab_string[check_result_path] = DEFAULT_CHECK_RESULT_PATH;
  _tab_string[log_archive_path] = DEFAULT_LOG_ARCHIVE_PATH;
}

/**
 *  Copy constructor.
 *
 *  @param[in] right Object to copy.
 */
configuration::configuration(configuration const& right) {
  operator=(right);
}

/**
 *  Destructor.
 */
configuration::~configuration() throw() {

}

/**
 *  Assignment operator.
 *
 *  @param[in] right Object to copy.
 *
 *  @return This object.
 */
configuration& configuration::operator=(configuration const& right) {
  if (this != &right) {
    _filename = right._filename;
    _cur_line = right._cur_line;
    _command_check_interval_is_seconds = right._command_check_interval_is_seconds;

    for (unsigned int i = 0; i < max_string; ++i){
      _tab_string[i] = right._tab_string[i];
    }
    for (unsigned int i = 0; i < max_ulong; ++i){
      _tab_ulong[i] = right._tab_ulong[i];
    }
    for (unsigned int i = 0; i < max_float; ++i){
      _tab_float[i] = right._tab_float[i];
    }
    for (unsigned int i = 0; i < max_int; ++i){
      _tab_int[i] = right._tab_int[i];
    }
    for (unsigned int i = 0; i < max_uint; ++i){
      _tab_uint[i] = right._tab_uint[i];
    }
    for (unsigned int i = 0; i < max_bool; ++i){
      _tab_bool[i] = right._tab_bool[i];
    }
  }
  return (*this);
}

/**
 *  Reset variable
 */
void configuration::reset() {
  _reset();

  set_temp_path(DEFAULT_TEMP_PATH);
  set_check_result_path(DEFAULT_CHECK_RESULT_PATH);
  set_log_archive_path(DEFAULT_LOG_ARCHIVE_PATH);
}

/**
 *  Parse configuration file
 *
 *  @param[in] filename configuration file
 */
void configuration::parse(std::string const& filename)
{
  std::ifstream ifs;
  ifs.open(filename.c_str());
  if (ifs.fail()) {
    throw (error() << "cannot open configuration file: `" << filename << "'");
  }

  _filename = filename;
  _command_check_interval_is_seconds = false;
  for (_cur_line = 1; !ifs.eof(); ++_cur_line) {
      std::string line = _getline(ifs);
      if (line == "" || line[0] == '#') {
      	continue;
      }

      size_t pos = line.find_first_of('=');
      if (pos == std::string::npos) {
      	throw (error() << "[" << _filename << ":" << _cur_line << "] bad variable name: `"
      	       << _filename << "'");
      }
      std::string key = line.substr(0, pos);
      methods::const_iterator it = _lst_method.find(_trim(key));
      if (it != _lst_method.end()) {
      	if (it->second != NULL) {
      	  std::string value = line.substr(pos + 1);
	  try {
	    it->second(_trim(value), *this);
	  }
	  catch (error const& e) {
	    throw (error() << "[" << _filename << ":" << _cur_line << "] " << e.what());
	  }
      	}
      }
      else if (!key.compare(0, 13, "host_perfdata") ||
      	       !key.compare(0, 16, "service_perfdata")) {
      	continue;
      }
      else {
      	throw (error() << "[" << _filename << ":" << _cur_line
      	       << "] unknown variable name: `" << key << "'");
      }
  }
  ifs.close();

  if (_tab_string[log_file] == "") {
    throw (error() << "log_file is not specified anywhere in `" << _filename << "'");
  }

  if (!get_use_timezone().empty()) {
    set_environment_var("TZ", get_use_timezone().c_str(), 1);
  }
  tzset();

  // adjust tweaks
  if(_tab_int[free_child_process_memory] == -1) {
    _tab_int[free_child_process_memory] = !_tab_bool[use_large_installation_tweaks];
  }
  if (_tab_int[child_processes_fork_twice] == -1) {
    _tab_int[child_processes_fork_twice] = !_tab_bool[use_large_installation_tweaks];
  }

  delete[] _mac->x[MACRO_MAINCONFIGFILE];
  _mac->x[MACRO_MAINCONFIGFILE] = my_strdup(_filename.c_str());

  // check path
  set_temp_path(get_temp_path());
  set_check_result_path(get_check_result_path());
  set_log_archive_path(get_log_archive_path());
}

/**
 *  Get the logging filename.
 *  @return The logging filename.
 */
std::string const& configuration::get_log_file() const throw() {
  return (_tab_string[log_file]);
}

/**
 *  Get the debug filename.
 *  @return The debug filename.
 */
std::string const& configuration::get_debug_file() const throw() {
  return (_tab_string[debug_file]);
}

/**
 *  Get the command filename.
 *  @return The command filename.
 */
std::string const& configuration::get_command_file() const throw() {
  return (_tab_string[command_file]);
}

/**
 *  Get the temporary filename.
 *  @return The temporary filename.
 */
std::string const& configuration::get_temp_file() const throw() {
  return (_tab_string[temp_file]);
}

/**
 *  Get the temporary path.
 *  @return The temporary path.
 */
std::string const& configuration::get_temp_path() const throw() {
  return (_tab_string[temp_path]);
}

/**
 *  Get The Check result path.
 *  @return the check result path.
 */
std::string const& configuration::get_check_result_path() const throw() {
  return (_tab_string[check_result_path]);
}

/**
 *  Get the global host event handler.
 *  @return The global host event handler.
 */
std::string const& configuration::get_global_host_event_handler() const throw() {
  return (_tab_string[global_host_event_handler]);
}

/**
 *  Get the global service event handler.
 *  @return The global service event handler.
 */
std::string const& configuration::get_global_service_event_handler() const throw() {
  return (_tab_string[global_service_event_handler]);
}

/**
 *  Get the ocsp command.
 *  @return The ocsp command.
 */
std::string const& configuration::get_ocsp_command() const throw() {
  return (_tab_string[ocsp_command]);
}

/**
 *  Get the ochp command.
 *  @return The ochp command.
 */
std::string const& configuration::get_ochp_command() const throw() {
  return (_tab_string[ochp_command]);
}

/**
 *  Get the logging archive path.
 *  @return The logging archive path.
 */
std::string const& configuration::get_log_archive_path() const throw() {
  return (_tab_string[log_archive_path]);
}

/**
 *  Get the p1 filename.
 *  @return The p1 filename.
 */
std::string const& configuration::get_p1_file() const throw() {
  return (_tab_string[p1_file]);
}

/**
 *  Get the illegal object characters.
 *  @return The illegal object characters.
 */
std::string const& configuration::get_illegal_object_chars() const throw() {
  return (_tab_string[illegal_object_chars]);
}

/**
 *  Get the illegal output characters.
 *  @return The illegal output characters.
 */
std::string const& configuration::get_illegal_output_chars() const throw() {
  return (_tab_string[illegal_output_chars]);
}

/**
 *  Get the use timezone.
 *  @return The use timezone.
 */
std::string const& configuration::get_use_timezone() const throw() {
  return (_tab_string[use_timezone]);
}

/**
 *  Get the additional freshness latency.
 *  @return The additional freshness latency.
 */
int configuration::get_additional_freshness_latency() const throw() {
  return (_tab_int[additional_freshness_latency]);
}

/**
 *  Get the debug level.
 *  @return The debug level.
 */
unsigned int configuration::get_debug_level() const throw() {
  return (_tab_uint[debug_level]);
}

/**
 *  Get the debug verbosity.
 *  @return The debug verbosity.
 */
unsigned int configuration::get_debug_verbosity() const throw() {
  return (_tab_uint[debug_verbosity]);
}

/**
 *  Get the command check interval.
 *  @return The command check interval.
 */
int configuration::get_command_check_interval() const throw() {
  return (_tab_int[command_check_interval]);
}

/**
 *  Get the external command buffer slots.
 *  @return The external command buffer slots.
 */
int configuration::get_external_command_buffer_slots() const throw() {
  return (_tab_int[external_command_buffer_slots]);
}

/**
 *  Get the get max service check spread.
 *  @return The get max service check spread.
 */
unsigned int configuration::get_max_service_check_spread() const throw() {
  return (_tab_uint[max_service_check_spread]);
}

/**
 *  Get the max host check spread.
 *  @return The max host check spread.
 */
unsigned int configuration::get_max_host_check_spread() const throw() {
  return (_tab_uint[max_host_check_spread]);
}

/**
 *  Get the max concurrent checks.
 *  @return The max concurrent chechs.
 */
unsigned int configuration::get_max_concurrent_checks() const throw() {
  return (_tab_uint[max_concurrent_checks]);
}

/**
 *  Get the max parallel service checks.
 *  @return The max parallel service checks.
 */
unsigned int configuration::get_max_parallel_service_checks() const throw() {
  return (_tab_uint[max_parallel_service_checks]);
}

/**
 *  Get the check reaper interval.
 *  @return The check readper interval.
 */
unsigned int configuration::get_check_reaper_interval() const throw() {
  return (_tab_uint[check_reaper_interval]);
}

/**
 *  Get the max check reaper time.
 *  @return The max check reaper time.
 */
unsigned int configuration::get_max_check_reaper_time() const throw() {
  return (_tab_uint[max_check_reaper_time]);
}

/**
 *  Get the interval length.
 *  @return The interval length.
 */
unsigned int configuration::get_interval_length() const throw() {
  return (_tab_uint[interval_length]);
}

/**
 *  Get the service freshness check interval.
 *  @return The service freshness check interval.
 */
unsigned int configuration::get_service_freshness_check_interval() const throw() {
  return (_tab_uint[service_freshness_check_interval]);
}

/**
 *  Get the host freshness check interval.
 *  @return The host freshness check interval.
 */
unsigned int configuration::get_host_freshness_check_interval() const throw() {
  return (_tab_uint[host_freshness_check_interval]);
}

/**
 *  Get the auto rescheduling interval.
 *  @return The auto rescheduling interval.
 */
unsigned int configuration::get_auto_rescheduling_interval() const throw() {
  return (_tab_uint[auto_rescheduling_interval]);
}

/**
 *  Get the auto rescheduling window.
 *  @return The auto rescheduling window.
 */
unsigned int configuration::get_auto_rescheduling_window() const throw() {
  return (_tab_uint[auto_rescheduling_window]);
}

/**
 *  Get the status update interval.
 *  @return The status update interval.
 */
unsigned int configuration::get_status_update_interval() const throw() {
  return (_tab_uint[status_update_interval]);
}

/**
 *  Get the time change threshold.
 *  @return The time change threshold.
 */
unsigned int configuration::get_time_change_threshold() const throw() {
  return (_tab_uint[time_change_threshold]);
}

/**
 *  Get the retention update interval.
 *  @return The retention update interval.
 */
unsigned int configuration::get_retention_update_interval() const throw() {
  return (_tab_uint[retention_update_interval]);
}

/**
 *  Get the retention scheduling horizon.
 *  @return The retention scheduling horizon.
 */
unsigned int configuration::get_retention_scheduling_horizon() const throw() {
  return (_tab_uint[retention_scheduling_horizon]);
}

/**
 *  Get the service check timeout.
 *  @return The service check timeout.
 */
unsigned int configuration::get_service_check_timeout() const throw() {
  return (_tab_uint[service_check_timeout]);
}

/**
 *  Get the host check timeout.
 *  @return The host check timeout.
 */
unsigned int configuration::get_host_check_timeout() const throw() {
  return (_tab_uint[host_check_timeout]);
}

/**
 *  Get the event handler timeout.
 *  @return the event handler timeout.
 */
unsigned int configuration::get_event_handler_timeout() const throw() {
  return (_tab_uint[event_handler_timeout]);
}

/**
 *  Get the notification timeout.
 *  @return The notification timeout.
 */
unsigned int configuration::get_notification_timeout() const throw() {
  return (_tab_uint[notification_timeout]);
}

/**
 *  Get the oscp timeout.
 *  @return The oscp timeout.
 */
unsigned int configuration::get_ocsp_timeout() const throw() {
  return (_tab_uint[ocsp_timeout]);
}

/**
 *  Get the ochp timeout.
 *  @return The ochp timeout.
 */
unsigned int configuration::get_ochp_timeout() const throw() {
  return (_tab_uint[ochp_timeout]);
}

/**
 *  Get the max debug file size.
 *  @return The max debug file size.
 */
unsigned long configuration::get_max_debug_file_size() const throw() {
  return (_tab_ulong[max_debug_file_size]);
}

/**
 *  Get the max check result file age.
 *  @return The max check result file age.
 */
unsigned long configuration::get_max_check_result_file_age() const throw() {
  return (_tab_ulong[max_check_result_file_age]);
}

/**
 *  Get the retained host attribute mask.
 *  @return The retained host attribute mask.
 */
unsigned long configuration::get_retained_host_attribute_mask() const throw() {
  return (_tab_ulong[retained_host_attribute_mask]);
}

/**
 *  Get the retained process host attribute mask.
 *  @return The retained process host attribute mask.
 */
unsigned long configuration::get_retained_process_host_attribute_mask() const throw() {
  return (_tab_ulong[retained_process_host_attribute_mask]);
}

/**
 *  Get the retained contact host attribute mask.
 *  @return The retained contact host attribute mask.
 */
unsigned long configuration::get_retained_contact_host_attribute_mask() const throw() {
  return (_tab_ulong[retained_contact_host_attribute_mask]);
}

/**
 *  Get the retained contact service attribute mask.
 *  @return The retained contact service attribute mask.
 */
unsigned long configuration::get_retained_contact_service_attribute_mask() const throw() {
  return (_tab_ulong[retained_contact_service_attribute_mask]);
}

/**
 *  Get the cached host check horizon.
 *  @return The cached host check horizon.
 */
unsigned long configuration::get_cached_host_check_horizon() const throw() {
  return (_tab_ulong[cached_host_check_horizon]);
}

/**
 *  Get the cached service check horizon.
 *  @return The cached service check horizon.
 */
unsigned long configuration::get_cached_service_check_horizon() const throw() {
  return (_tab_ulong[cached_service_check_horizon]);
}

/**
 *  Get the event broker options.
 *  @return The event broker options.
 */
unsigned long configuration::get_event_broker_options() const throw() {
  return (_tab_ulong[event_broker_options]);
}

/**
 *  Get the use syslog.
 *  @return The use syslog.
 */
bool configuration::get_use_syslog() const throw() {
  return (_tab_bool[use_syslog]);
}

/**
 *  Get the logging notifications.
 *  @return Th logging notifications.
 */
bool configuration::get_log_notifications() const throw() {
  return (_tab_bool[log_notifications]);
}

/**
 *  Get the logging service retries.
 *  @return The logging service retries.
 */
bool configuration::get_log_service_retries() const throw() {
  return (_tab_bool[log_service_retries]);
}

/**
 *  Get the logging host retries.
 *  @return The logging host retries.
 */
bool configuration::get_log_host_retries() const throw() {
  return (_tab_bool[log_host_retries]);
}

/**
 *  Get the logging event handlers.
 *  @return T
 */
bool configuration::get_log_event_handlers() const throw() {
  return (_tab_bool[log_event_handlers]);
}

/**
 *  Get the logging event handlers.
 *  @return The logging event handlers.
 */
bool configuration::get_log_external_commands() const throw() {
  return (_tab_bool[log_external_commands]);
}

/**
 *  Get the logging passive checks.
 *  @return The logging passive checks.
 */
bool configuration::get_log_passive_checks() const throw() {
  return (_tab_bool[log_passive_checks]);
}

/**
 *  Get the logging initial states.
 *  @return The logging initial states.
 */
bool configuration::get_log_initial_states() const throw() {
  return (_tab_bool[log_initial_states]);
}

/**
 *  Get the retain state information.
 *  @return The retain state information.
 */
bool configuration::get_retain_state_information() const throw() {
  return (_tab_bool[retain_state_information]);
}

/**
 *  Get the use retained program state.
 *  @return The use retained program state.
 */
bool configuration::get_use_retained_program_state() const throw() {
  return (_tab_bool[use_retained_program_state]);
}

/**
 *  Get the use retained schedyling info.
 *  @return The use retained schedyling info.
 */
bool configuration::get_use_retained_scheduling_info() const throw() {
  return (_tab_bool[use_retained_scheduling_info]);
}

/**
 *  Get the obsess over services.
 *  @return The obsess over services.
 */
bool configuration::get_obsess_over_services() const throw() {
  return (_tab_bool[obsess_over_services]);
}

/**
 *  Get the obsess over hosts.
 *  @return The obsess over hosts.
 */
bool configuration::get_obsess_over_hosts() const throw() {
  return (_tab_bool[obsess_over_hosts]);
}

/**
 *  Get the translate passive host checks.
 *  @return The translate passive host checks.
 */
bool configuration::get_translate_passive_host_checks() const throw() {
  return (_tab_bool[translate_passive_host_checks]);
}

/**
 *  Get the passive host checks are soft
 *  @return The passive host checks are soft
 */
bool configuration::get_passive_host_checks_are_soft() const throw() {
  return (_tab_bool[passive_host_checks_are_soft]);
}

/**
 *  Get the use aggressive host checking.
 *  @return The use aggressive host checking.
 */
bool configuration::get_use_aggressive_host_checking() const throw() {
  return (_tab_bool[use_aggressive_host_checking]);
}

/**
 *  Get the enable predictive host dependency checks.
 *  @return The enable predictive host dependency checks.
 */
bool configuration::get_enable_predictive_host_dependency_checks() const throw() {
  return (_tab_bool[enable_predictive_host_dependency_checks]);
}

/**
 *  Get the enable predictive service dependency checks.
 *  @return The enable predictive service dependency checks.
 */
bool configuration::get_enable_predictive_service_dependency_checks() const throw() {
  return (_tab_bool[enable_predictive_service_dependency_checks]);
}

/**
 *  Get the soft state dependencies.
 *  @return The soft state dependencies.
 */
bool configuration::get_soft_state_dependencies() const throw() {
  return (_tab_bool[soft_state_dependencies]);
}

/**
 *  Get the enable event handlers.
 *  @return The enable event handlers.
 */
bool configuration::get_enable_event_handlers() const throw() {
  return (_tab_bool[enable_event_handlers]);
}

/**
 *  Get the enable notifications.
 *  @return The enable notifications.
 */
bool configuration::get_enable_notifications() const throw() {
  return (_tab_bool[enable_notifications]);
}

/**
 *  Get the execute service checks.
 *  @return The execute service checks.
 */
bool configuration::get_execute_service_checks() const throw() {
  return (_tab_bool[execute_service_checks]);
}

/**
 *  Get the accept passive service checks.
 *  @return The accept passive service checks.
 */
bool configuration::get_accept_passive_service_checks() const throw() {
  return (_tab_bool[accept_passive_service_checks]);
}

/**
 *  Get the execute host checks.
 *  @return The execute host checks.
 */
bool configuration::get_execute_host_checks() const throw() {
  return (_tab_bool[execute_host_checks]);
}

/**
 *  Get the accept passive host checks.
 *  @return The accept passive host checks.
 */
bool configuration::get_accept_passive_host_checks() const throw() {
  return (_tab_bool[accept_passive_host_checks]);
}

/**
 *  Get the check external commands.
 *  @return The check external commands.
 */
bool configuration::get_check_external_commands() const throw() {
  return (_tab_bool[check_external_commands]);
}

/**
 *  Get the check orphaned services.
 *  @return The check orphaned services.
 */
bool configuration::get_check_orphaned_services() const throw() {
  return (_tab_bool[check_orphaned_services]);
}

/**
 *  Get the check orphaned hosts.
 *  @return The check orphaned hosts.
 */
bool configuration::get_check_orphaned_hosts() const throw() {
  return (_tab_bool[check_orphaned_hosts]);
}

/**
 *  Get the check service freshness.
 *  @return The check service freshness.
 */
bool configuration::get_check_service_freshness() const throw() {
  return (_tab_bool[check_service_freshness]);
}

/**
 *  Get the check host freshness.
 *  @return The check host freshness.
 */
bool configuration::get_check_host_freshness() const throw() {
  return (_tab_bool[check_host_freshness]);
}

/**
 *  Get the auto reschedule checks.
 *  @return The auto reschedule checks.
 */
bool configuration::get_auto_reschedule_checks() const throw() {
  return (_tab_bool[auto_reschedule_checks]);
}

/**
 *  Get the process performance data.
 *  @return The process performance data.
 */
bool configuration::get_process_performance_data() const throw() {
  return (_tab_bool[process_performance_data]);
}

/**
 *  Get the enable flap detection.
 *  @return The enable flap detection.
 */
bool configuration::get_enable_flap_detection() const throw() {
  return (_tab_bool[enable_flap_detection]);
}

/**
 *  Get the enable failure prediction.
 *  @return The enable failure prediction.
 */
bool configuration::get_enable_failure_prediction() const throw() {
  return (_tab_bool[enable_failure_prediction]);
}

/**
 *  Get the use regexp matching.
 *  @return The use regexp matching.
 */
bool configuration::get_use_regexp_matches() const throw() {
  return (_tab_bool[use_regexp_matches]);
}

/**
 *  Get the use true regexp matching.
 *  @return The use true regexp matching.
 */
bool configuration::get_use_true_regexp_matching() const throw() {
  return (_tab_bool[use_true_regexp_matching]);
}

/**
 *  Get the use large installation tweaks.
 *  @return The use large installation tweaks.
 */
bool configuration::get_use_large_installation_tweaks() const throw() {
  return (_tab_bool[use_large_installation_tweaks]);
}

/**
 *  Get the enable environnement macros.
 *  @return The enable environnement macros.
 */
bool configuration::get_enable_environment_macros() const throw() {
  return (_tab_bool[enable_environment_macros]);
}

/**
 *  Get the free child process memory.
 *  @return The free child process memory.
 */
bool configuration::get_free_child_process_memory() const throw() {
  return (_tab_int[free_child_process_memory]);
}

/**
 *  Get the child processes fork twice.
 *  @return The child processes fork twice.
 */
bool configuration::get_child_processes_fork_twice() const throw() {
  return (_tab_int[child_processes_fork_twice]);
}

/**
 *  Get the enable embedded perl.
 *  @return The enable embedded perl.
 */
bool configuration::get_enable_embedded_perl() const throw() {
  return (_tab_bool[enable_embedded_perl]);
}

/**
 *  Get the use embedded perl implicitly.
 *  @return The use embedded perl implicitly.
 */
bool configuration::get_use_embedded_perl_implicitly() const throw() {
  return (_tab_bool[use_embedded_perl_implicitly]);
}

/**
 *  Get the allow empty hostgroup assignment.
 *  @return The allow empty hostgroup assignment.
 */
bool configuration::get_allow_empty_hostgroup_assignment() const throw() {
  return (_tab_bool[allow_empty_hostgroup_assignment]);
}

/**
 *  Get the sleep time.
 *  @return The sleep time.
 */
float configuration::get_sleep_time() const throw() {
  return (_tab_float[sleep_time]);
}

/**
 *  Get the low service flap threshold.
 *  @return The low service flap threshold.
 */
float configuration::get_low_service_flap_threshold() const throw() {
  return (_tab_float[low_service_flap_threshold]);
}

/**
 *  Get the hight service flap threshold.
 *  @return The hight service flap threshold.
 */
float configuration::get_high_service_flap_threshold() const throw() {
  return (_tab_float[high_service_flap_threshold]);
}

/**
 *  Get the low host flap threshold.
 *  @return The low host flap threshold.
 */
float configuration::get_low_host_flap_threshold() const throw() {
  return (_tab_float[low_host_flap_threshold]);
}

/**
 *  Get the high host flap threshold.
 *  @return The high host flap threshold.
 */
float configuration::get_high_host_flap_threshold() const throw() {
  return (_tab_float[high_host_flap_threshold]);
}

/**
 *  Get the date format.
 *  @return The date format.
 */
configuration::e_date_format configuration::get_date_format() const throw() {
  return (static_cast<e_date_format>(_tab_uint[date_format]));
}

/**
 *  Get the logging rotation method.
 *  @return The logging rotation method.
 */
configuration::e_log_rotation configuration::get_log_rotation_method() const throw() {
  return (static_cast<e_log_rotation>(_tab_uint[log_rotation_method]));
}

/**
 *  Get the service inter check delay method.
 *  @return The service inter check delay method.
 */
configuration::e_inter_check_delay configuration::get_service_inter_check_delay_method() const throw() {
  return (static_cast<e_inter_check_delay>(_tab_uint[service_inter_check_delay_method]));
}

/**
 *  Get the host inter check delay method.
 *  @return The host inter check delay method.
 */
configuration::e_inter_check_delay configuration::get_host_inter_check_delay_method() const throw() {
  return (static_cast<e_inter_check_delay>(_tab_uint[host_inter_check_delay_method]));
}

/**
 *  Get the service interleave factor method.
 *  @return The service interleave factor method.
 */
configuration::e_interleave_factor configuration::get_service_interleave_factor_method() const throw() {
  return (static_cast<e_interleave_factor>(_tab_uint[service_interleave_factor_method]));
}

/**
 *  Set the logging filename.
 *  @param[in] value The filename.
 */
void configuration::set_log_file(std::string const& value) {
  if (pathconf(value.c_str(), _PC_PATH_MAX) == -1) {
    throw (error() << "log_file: invalid value");
  }
  _tab_string[log_file] = value;

  delete[] _mac->x[MACRO_LOGFILE];
  _mac->x[MACRO_LOGFILE] = my_strdup(value.c_str());
}

/**
 *  Set the debug filename.
 *  @param[in] value The filename
 */
void configuration::set_debug_file(std::string const& value) {
  if (pathconf(value.c_str(), _PC_PATH_MAX) == -1) {
    throw (error() << "debuf_file: invalid value");
  }
  _tab_string[debug_file] = value;
}

/**
 *  Set the command filename.
 *  @param[in] value The filename.
 */
void configuration::set_command_file(std::string const& value) {
  if (pathconf(value.c_str(), _PC_PATH_MAX) == -1) {
    throw (error() << "command_file: invalid value");
  }
  _tab_string[command_file] = value;

  delete[] _mac->x[MACRO_COMMANDFILE];
  _mac->x[MACRO_COMMANDFILE] = my_strdup(value.c_str());
}

/**
 *  Set the temporary filename.
 *  @param[in] value The filename.
 */
void configuration::set_temp_file(std::string const& value) {
  if (pathconf(value.c_str(), _PC_PATH_MAX) == -1) {
    throw (error() << "temp_file: invalid value");
  }
  _tab_string[temp_file] = value;

  delete[] _mac->x[MACRO_TEMPFILE];
  _mac->x[MACRO_TEMPFILE] = my_strdup(value.c_str());
}

/**
 *  Set the temporary path.
 *  @param[in] value The path.
 */
void configuration::set_temp_path(std::string const& value) {
  struct stat stat_info;
  if (stat(value.c_str(), &stat_info) == -1 ||
      !S_ISDIR(stat_info.st_mode)) {
    throw (error() << "temp_path: invalid value");
  }
  _tab_string[temp_path] = value;

  delete[] _mac->x[MACRO_TEMPPATH];
  _mac->x[MACRO_TEMPPATH] = my_strdup(value.c_str());
}

/**
 *  Set the check result path.
 *  @param[in] value The path.
 */
void configuration::set_check_result_path(std::string const& value) {
  struct stat stat_info;
  if (stat(value.c_str(), &stat_info) == -1 ||
      !S_ISDIR(stat_info.st_mode)) {
    throw (error() << "check_result_path: invalid value");
  }
  _tab_string[check_result_path] = value;
}

/**
 *  Set the global host event handler.
 *  @param[in] value The event handler.
 */
void configuration::set_global_host_event_handler(std::string const& value) {
  _tab_string[global_host_event_handler] = value;
}

/**
 *  Set the service event handler.
 *  @param[in] value The event handler.
 */
void configuration::set_global_service_event_handler(std::string const& value) {
  _tab_string[global_service_event_handler] = value;
}

/**
 *  Set the ocsp command.
 *  @param[in] value The command.
 */
void configuration::set_ocsp_command(std::string const& value) {
  _tab_string[ocsp_command] = value;
}

/**
 *  Set the ochp command.
 *  @param[in] value The command.
 */
void configuration::set_ochp_command(std::string const& value) {
  _tab_string[ochp_command] = value;
}

/**
 *  Set the logging archive path.
 *  @param[in] value The path.
 */
void configuration::set_log_archive_path(std::string const& value) {
  struct stat stat_info;
  if (stat(value.c_str(), &stat_info) == -1 ||
      !S_ISDIR(stat_info.st_mode)) {
    throw (error() << "log_archive_path: invalid value");
  }
  _tab_string[log_archive_path] = value;
}

/**
 *  Set the p1 filename.
 *  @param[in] value The filename.
 */
void configuration::set_p1_file(std::string const& value) {
  if (pathconf(value.c_str(), _PC_PATH_MAX) == -1) {
    throw (error() << "p1_file: invalid value");
  }
  _tab_string[p1_file] = value;
}

/**
 *  Set the illegal object characters.
 *  @param[in] value The illegal object characters.
 */
void configuration::set_illegal_object_chars(std::string const& value) {
  _tab_string[illegal_object_chars] = value;
}

/**
 *  Set the illegal output characters.
 *  @param[in] value The illegal output characters.
 */
void configuration::set_illegal_output_chars(std::string const& value) {
  _tab_string[illegal_output_chars] = value;
}

/**
 *  Set the use timezone.
 *  @param[in] value The timezone.
 */
void configuration::set_use_timezone(std::string const& value) {
  _tab_string[use_timezone] = value;
}

/**
 *  Set the additional freshness latency.
 *  @param[in] value The additional freshness latency.
 */
void configuration::set_additional_freshness_latency(int value) {
  _tab_int[additional_freshness_latency] = value;
}

/**
 *  Set the debug level.
 *  @param[in] value The level.
 */
void configuration::set_debug_level(unsigned int value) {
  _tab_uint[debug_level] = value;
}

/**
 *  Set the debug verbosity.
 *  @param[in] value The verbosity.
 */
void configuration::set_debug_verbosity(unsigned int value) {
  _tab_uint[debug_verbosity] = value;
}

/**
 *  Set the command check interval.
 *  @param[in] value The check interval.
 */
void configuration::set_command_check_interval(int value) {
  _tab_int[command_check_interval] = value;
  if (_tab_int[command_check_interval] < -1 ||
      _tab_int[command_check_interval] == 0) {
    throw (error() << "command_check_interval: invalid value");
  }

  // adjust command check interval
  if (_command_check_interval_is_seconds == false &&
      _tab_int[command_check_interval] != -1) {
    _tab_int[command_check_interval] *= _tab_uint[interval_length];
  }
}

/**
 *  Set the command check interval.
 *  @param[in] value The check interval.
 */
void configuration::set_command_check_interval(std::string const& value) {
  size_t pos = value.find('s');
  std::string val = value;

  if (pos == std::string::npos) {
    _command_check_interval_is_seconds = false;
  }
  else if (pos == val.size() - 1) {
    _command_check_interval_is_seconds = true;
    val.erase(val.begin() + pos);
  }

  cpp_suck<int, &configuration::set_command_check_interval>::set_generic(val, *this);
}

/**
 *  Set the external command buffer slots.
 *  @param[in] value The external command buffer slots.
 */
void configuration::set_external_command_buffer_slots(int value) {
  _tab_int[external_command_buffer_slots] = value;
}

/**
 *  Set the max service check spread.
 *  @param[in] value The max service check spread.
 */
void configuration::set_max_service_check_spread(unsigned int value) {
  if (value == 0) {
    throw (error() << "max_service_check_spread: invalid value");
  }
  _tab_uint[max_service_check_spread] = value;
}

/**
 *  Set the max host check spread.
 *  @param[in] value The max host check spread.
 */
void configuration::set_max_host_check_spread(unsigned int value) {
  if (value == 0) {
    throw (error() << "max_host_check_spread: invalid value");
  }
  _tab_uint[max_host_check_spread] = value;
}

/**
 *  Set the max concurrent checks.
 *  @param[in] value The max concurrents checks.
 */
void configuration::set_max_concurrent_checks(unsigned int value) {
  _tab_uint[max_concurrent_checks] = value;
}

/**
 *  Set the max parallel service checks.
 *  @param[in] value The max parallel service checks.
 */
void configuration::set_max_parallel_service_checks(unsigned int value) {
  _tab_uint[max_parallel_service_checks] = value;
}

/**
 *  Set the check reaper interval.
 *  @param[in] value The reaper interval.
 */
void configuration::set_check_reaper_interval(unsigned int value) {
  if (value == 0) {
    throw (error() << "check_reaper_interval: invalid value");
  }
  _tab_uint[check_reaper_interval] = value;
}

/**
 *  Set the max check reaper time.
 *  @param[in] value The reaper time.
 */
void configuration::set_max_check_reaper_time(unsigned int value) {
  if (value == 0) {
    throw (error() << "max_check_reaper_time: invalid value");
  }
  _tab_uint[max_check_reaper_time] = value;
}

/**
 *  Set the interval length.
 *  @param[in] value The interval length.
 */
void configuration::set_interval_length(unsigned int value) {
  if (value == 0) {
    throw (error() << "interval_length: invalid value");
  }

  if (_command_check_interval_is_seconds == false &&
      _tab_int[command_check_interval] != -1) {
    _tab_int[command_check_interval] /= _tab_uint[interval_length];
    _tab_uint[interval_length] = value;
    _tab_int[command_check_interval] *= _tab_uint[interval_length];
  }
  else {
    _tab_uint[interval_length] = value;
  }
}

/**
 *  Set the service freshness check interval.
 *  @param[in] value The check interval.
 */
void configuration::set_service_freshness_check_interval(unsigned int value) {
  if (value == 0) {
    throw (error() << "service_freshness_check_interval: invalid value");
  }
  _tab_uint[service_freshness_check_interval] = value;
}

/**
 *  Set the host freshness check interval.
 *  @param[in] value The check interval.
 */
void configuration::set_host_freshness_check_interval(unsigned int value) {
  if (value == 0) {
    throw (error() << "host_freshness_check_interval: invalid value");
  }
  _tab_uint[host_freshness_check_interval] = value;
}

/**
 *  Set the auto rescheduling interval.
 *  @param[in] value The rescheduling interval.
 */
void configuration::set_auto_rescheduling_interval(unsigned int value) {
  if (value == 0) {
    throw (error() << "auto_rescheduling_interval: invalid value");
  }
  _tab_uint[auto_rescheduling_interval] = value;
}

/**
 *  Set the auto rescheduling window.
 *  @param[in] value The rescheduling window.
 */
void configuration::set_auto_rescheduling_window(unsigned int value) {
  if (value == 0) {
    throw (error() << "auto_rescheduling_window: invalid value");
  }
  _tab_uint[auto_rescheduling_window] = value;
}

/**
 *  Set the status update interval.
 *  @param[in] value The status update interval.
 */
void configuration::set_status_update_interval(unsigned int value) {
  if (value < 2) {
    throw (error() << "status_update_interval: invalid value");
  }
  _tab_uint[status_update_interval] = value;
}

/**
 *  Set the time change threshold.
 *  @param[in] value The time change threshold.
 */
void configuration::set_time_change_threshold(unsigned int value) {
  if (value < 6) {
    throw (error() << "time_change_threshold: invalid value");
  }
  _tab_uint[time_change_threshold] = value;
}

/**
 *  Set the retention update interval.
 *  @param[in] value The update interval.
 */
void configuration::set_retention_update_interval(unsigned int value) {
  if (value == 0) {
    throw (error() << "retention_update_interval: invalid value");
  }
  _tab_uint[retention_update_interval] = value;
}

/**
 *  Set the retention scheduling horizon.
 *  @param[in] value The retention scheduling horizon.
 */
void configuration::set_retention_scheduling_horizon(unsigned int value) {
  if (value == 0) {
    throw (error() << "retention_scheduling_horizon: invalid value");
  }
  _tab_uint[retention_scheduling_horizon] = value;
}

/**
 *  Set the service check timeout.
 *  @param[in] value The timeout.
 */
void configuration::set_service_check_timeout(unsigned int value) {
  if (value == 0) {
    throw (error() << "service_check_timeout: invalid value");
  }
  _tab_uint[service_check_timeout] = value;
}

/**
 *  Set the host check timeout.
 *  @param[in] value The timeout.
 */
void configuration::set_host_check_timeout(unsigned int value) {
  if (value == 0) {
    throw (error() << "host_check_timeout: invalid value");
  }
  _tab_uint[host_check_timeout] = value;
}

/**
 *  Set the event handler timeout.
 *  @param[in] value The timeout.
 */
void configuration::set_event_handler_timeout(unsigned int value) {
  if (value == 0) {
    throw (error() << "event_handler_timeout: invalid value");
  }
  _tab_uint[event_handler_timeout] = value;
}

/**
 *  Set the notification timeout.
 *  @param[in] value The timeout.
 */
void configuration::set_notification_timeout(unsigned int value) {
  if (value == 0) {
    throw (error() << "notification_timeout: invalid value");
  }
  _tab_uint[notification_timeout] = value;
}

/**
 *  Set the ocsp timeout.
 *  @param[in] value The timeout.
 */
void configuration::set_ocsp_timeout(unsigned int value) {
  if (value == 0) {
    throw (error() << "ocsp_timeout: invalid value");
  }
  _tab_uint[ocsp_timeout] = value;
}

/**
 *  Set the ochp timeout.
 *  @param[in] value The timeout.
 */
void configuration::set_ochp_timeout(unsigned int value) {
  if (value == 0) {
    throw (error() << "ochp_timeout: invalid value");
  }
  _tab_uint[ochp_timeout] = value;
}

/**
 *  Set the max debug file size.
 *  @param[in] value The size.
 */
void configuration::set_max_debug_file_size(unsigned long value) {
  _tab_ulong[max_debug_file_size] = value;
}

/**
 *  Set the max check result file age.
 *  @param[in] value The max check result file age.
 */
void configuration::set_max_check_result_file_age(unsigned long value) {
  _tab_ulong[max_check_result_file_age] = value;
}

/**
 *  Set the retained host attribute mask.
 *  @param[in] value The attribute mask.
 */
void configuration::set_retained_host_attribute_mask(unsigned long value) {
  _tab_ulong[retained_host_attribute_mask] = value;
}

/**
 *  Set the retained process host attribute mask.
 *  @param[in] value The attribute mask.
 */
void configuration::set_retained_process_host_attribute_mask(unsigned long value) {
  _tab_ulong[retained_process_host_attribute_mask] = value;
}

/**
 *  Set the retained contact host attribute mask.
 *  @param[in] value The attribute mask.
 */
void configuration::set_retained_contact_host_attribute_mask(unsigned long value) {
  _tab_ulong[retained_contact_host_attribute_mask] = value;
}

/**
 *  Set the retained contact service attribute mask.
 *  @param[in] value The attribute mask.
 */
void configuration::set_retained_contact_service_attribute_mask(unsigned long value) {
  _tab_ulong[retained_contact_service_attribute_mask] = value;
}

/**
 *  Set the cached host check horizon.
 *  @param[in] value The cached host check horizon.
 */
void configuration::set_cached_host_check_horizon(unsigned long value) {
  _tab_ulong[cached_host_check_horizon] = value;
}

/**
 *  Set the cached service check horizon.
 *  @param[in] value The cached service check horizon.
 */
void configuration::set_cached_service_check_horizon(unsigned long value) {
  _tab_ulong[cached_service_check_horizon] = value;
}

/**
 *  Set the event broker options.
 *  @param[in] value The options.
 */
void configuration::set_event_broker_options(unsigned long value) {
  _tab_ulong[event_broker_options] = value;
}

/**
 *  Set the event broker options.
 *  @param[in] value The options.
 */
void configuration::set_event_broker_options(std::string const& value) {
  if (value == "-1") {
    _tab_ulong[event_broker_options] = BROKER_EVERYTHING;
  }
  else {
    cpp_suck<unsigned long, &configuration::set_event_broker_options>::set_generic(value, *this);
  }
}

/**
 *  Set the use syslog.
 *  @param[in] value The use syslog.
 */
void configuration::set_use_syslog(bool value) {
  _tab_bool[use_syslog] = value;
}

/**
 *  Set the logging notifications.
 *  @param[in] value The logging notifications.
 */
void configuration::set_log_notifications(bool value) {
  _tab_bool[log_notifications] = value;
}

/**
 *  Set the logging service retries.
 *  @param[in] value The logging service retries.
 */
void configuration::set_log_service_retries(bool value) {
  _tab_bool[log_service_retries] = value;
}

/**
 *  Set the logging host retries.
 *  @param[in] value The logging host retries.
 */
void configuration::set_log_host_retries(bool value) {
  _tab_bool[log_host_retries] = value;
}

/**
 *  Set the logging event handlers.
 *  @param[in] value The logging event handlers.
 */
void configuration::set_log_event_handlers(bool value) {
  _tab_bool[log_event_handlers] = value;
}

/**
 *  Set the logging external commands.
 *  @param[in] value The logging external commands.
 */
void configuration::set_log_external_commands(bool value) {
  _tab_bool[log_external_commands] = value;
}

/**
 *  Set the logging passive checks.
 *  @param[in] value The logging passive checks.
 */
void configuration::set_log_passive_checks(bool value) {
  _tab_bool[log_passive_checks] = value;
}

/**
 *  Set the logging initial states.
 *  @param[in] value The logging initial states.
 */
void configuration::set_log_initial_states(bool value) {
  _tab_bool[log_initial_states] = value;
}

/**
 *  Set the retain state information.
 *  @param[in] value The retain state information.
 */
void configuration::set_retain_state_information(bool value) {
  _tab_bool[retain_state_information] = value;
}

/**
 *  Set the use retained program state.
 *  @param[in] value The retained program state.
 */
void configuration::set_use_retained_program_state(bool value) {
  _tab_bool[use_retained_program_state] = value;
}

/**
 *  Set the use retained scheduling info.
 *  @param[in] value The use retained scheduling info.
 */
void configuration::set_use_retained_scheduling_info(bool value) {
  _tab_bool[use_retained_scheduling_info] = value;
}

/**
 *  Set the obsess over services.
 *  @param[in] value The obsess over services.
 */
void configuration::set_obsess_over_services(bool value) {
  _tab_bool[obsess_over_services] = value;
}

/**
 *  Set the obsess over hosts.
 *  @param[in] value The obsess over hosts.
 */
void configuration::set_obsess_over_hosts(bool value) {
  _tab_bool[obsess_over_hosts] = value;
}

/**
 *  Set the translate passive host checks.
 *  @param[in] value The translate passive host checks.
 */
void configuration::set_translate_passive_host_checks(bool value) {
  _tab_bool[translate_passive_host_checks] = value;
}

/**
 *  Set the passive host checks are soft.
 *  @param[in] value The passive host checks are soft.
 */
void configuration::set_passive_host_checks_are_soft(bool value) {
  _tab_bool[passive_host_checks_are_soft] = value;
}

/**
 *  Set the use aggressive host checking.
 *  @param[in] value The use aggressive host checking.
 */
void configuration::set_use_aggressive_host_checking(bool value) {
  _tab_bool[use_aggressive_host_checking] = value;
}

/**
 *  Set the enable predictive host dependency checks.
 *  @param[in] value The enable predictive host dependency chehcks.
 */
void configuration::set_enable_predictive_host_dependency_checks(bool value) {
  _tab_bool[enable_predictive_host_dependency_checks] = value;
}

/**
 *  Set the enable predictive service dependency checks.
 *  @param[in] value The enable predictive service dependency checks.
 */
void configuration::set_enable_predictive_service_dependency_checks(bool value) {
  _tab_bool[enable_predictive_service_dependency_checks] = value;
}

/**
 *  Set the soft state dependencies.
 *  @param[in] value The soft state dependencies.
 */
void configuration::set_soft_state_dependencies(bool value) {
  _tab_bool[soft_state_dependencies] = value;
}

/**
 *  Set the enable event handlers.
 *  @param[in] value The enable event handlers.
 */
void configuration::set_enable_event_handlers(bool value) {
  _tab_bool[enable_event_handlers] = value;
}

/**
 *  Set the enable notifications.
 *  @param[in] value The enable notifications.
 */
void configuration::set_enable_notifications(bool value) {
  _tab_bool[enable_notifications] = value;
}

/**
 *  Set the execute service checks.
 *  @param[in] value The execute service checks.
 */
void configuration::set_execute_service_checks(bool value) {
  _tab_bool[execute_service_checks] = value;
}

/**
 *  Set the accept passive service checks.
 *  @param[in] value The accept passive service checks.
 */
void configuration::set_accept_passive_service_checks(bool value) {
  _tab_bool[accept_passive_service_checks] = value;
}

/**
 *  Set the execute host checks.
 *  @param[in] value The execute host checks.
 */
void configuration::set_execute_host_checks(bool value) {
  _tab_bool[execute_host_checks] = value;
}

/**
 *  Set the accept passive host checks.
 *  @param[in] value The accept passive host checks.
 */
void configuration::set_accept_passive_host_checks(bool value) {
  _tab_bool[accept_passive_host_checks] = value;
}

/**
 *  Set the check external commands.
 *  @param[in] value The check external commands.
 */
void configuration::set_check_external_commands(bool value) {
  _tab_bool[check_external_commands] = value;
}

/**
 *  Set the check orphaned services.
 *  @param[in] value The check orphaned services.
 */
void configuration::set_check_orphaned_services(bool value) {
  _tab_bool[check_orphaned_services] = value;
}

/**
 *  Set the check orphaned hosts.
 *  @param[in] value The check orphaned hosts.
 */
void configuration::set_check_orphaned_hosts(bool value) {
  _tab_bool[check_orphaned_hosts] = value;
}

/**
 *  Set the check service freshness.
 *  @param[in] value The check service freshness.
 */
void configuration::set_check_service_freshness(bool value) {
  _tab_bool[check_service_freshness] = value;
}

/**
 *  Set the check host freshness.
 *  @param[in] value The check host freshness.
 */
void configuration::set_check_host_freshness(bool value) {
  _tab_bool[check_host_freshness] = value;
}

/**
 *  Set the auto reschedule checks.
 *  @param[in] value The auto reschedule checks.
 */
void configuration::set_auto_reschedule_checks(bool value) {
  _tab_bool[auto_reschedule_checks] = value;
}

/**
 *  Set the process performance data.
 *  @param[in] value The process performance data.
 */
void configuration::set_process_performance_data(bool value) {
  _tab_bool[process_performance_data] = value;
}

/**
 *  Set the enable flap detection.
 *  @param[in] value The enable flap detection.
 */
void configuration::set_enable_flap_detection(bool value) {
  _tab_bool[enable_flap_detection] = value;
}

/**
 *  Set the enable failure prediction.
 *  @param[in] value The enable failure prediction.
 */
void configuration::set_enable_failure_prediction(bool value) {
  _tab_bool[enable_failure_prediction] = value;
}

/**
 *  Set the use regexp matches.
 *  @param[in] value The use regexp matches.
 */
void configuration::set_use_regexp_matches(bool value) {
  _tab_bool[use_regexp_matches] = value;
}

/**
 *  Set the use true regexp matching.
 *  @param[in] value The use true regexp matching.
 */
void configuration::set_use_true_regexp_matching(bool value) {
  _tab_bool[use_true_regexp_matching] = value;
}

/**
 *  Set the use large installation tweaks.
 *  @param[in] value The use large installation tweaks.
 */
void configuration::set_use_large_installation_tweaks(bool value) {
  _tab_bool[use_large_installation_tweaks] = value;
}

/**
 *  Set the enable environment macros.
 *  @param[in] value The enable environment macros.
 */
void configuration::set_enable_environment_macros(bool value) {
  _tab_bool[enable_environment_macros] = value;
}

/**
 *  Set the free child process memory.
 *  @param[in] value The free child process memory.
 */
void configuration::set_free_child_process_memory(bool value) {
  _tab_int[free_child_process_memory] = value;
}

/**
 *  Set the child processes fork twice.
 *  @param[in] value The child processes fork twice.
 */
void configuration::set_child_processes_fork_twice(bool value) {
  _tab_int[child_processes_fork_twice] = value;
}

/**
 *  Set the enable embedded perl.
 *  @param[in] value The enable embedded perl.
 */
void configuration::set_enable_embedded_perl(bool value) {
  _tab_bool[enable_embedded_perl] = value;
}

/**
 *  Set the use embedded perl implicitly.
 *  @param[in] value The use embedded perl implicitly.
 */
void configuration::set_use_embedded_perl_implicitly(bool value) {
  _tab_bool[use_embedded_perl_implicitly] = value;
}

/**
 *  Set the allow empty hostgroup assignment.
 *  @param[in] value The allow empty hostgroup assignment.
 */
void configuration::set_allow_empty_hostgroup_assignment(bool value) {
  _tab_bool[allow_empty_hostgroup_assignment] = value;
}

/**
 *  Set the sleep time.
 *  @param[in] value The sleep time.
 */
void configuration::set_sleep_time(float value) {
  if (value <= 0.0){
    throw (error() << "sleep_time: invalid value.");
  }
  _tab_float[sleep_time] = value;
}

/**
 *  Set the low service flap threshold.
 *  @param[in] value The low service flap threshold.
 */
void configuration::set_low_service_flap_threshold(float value) {
  if (value <= 0.0 || value >= 100.0) {
    throw (error() << "low_service_flap_threshold: invalid value.");
  }
  _tab_float[low_service_flap_threshold] = value;
}

/**
 *  Set the high service flap threshold.
 *  @param[in] value The high service flap threshold.
 */
void configuration::set_high_service_flap_threshold(float value) {
  if (value <= 0.0 || value >= 100.0) {
    throw (error() << "high_service_flap_threshold: invalid value.");
  }
  _tab_float[high_service_flap_threshold] = value;
}

/**
 *  Set the low host flap threshold.
 *  @param[in] value The low host flap threshold.
 */
void configuration::set_low_host_flap_threshold(float value) {
  if (value <= 0.0 || value >= 100.0) {
    throw (error() << "low_host_flap_threshold: invalid value.");
  }
  _tab_float[low_host_flap_threshold] = value;
}

/**
 *  Set the high host flap threshold.
 *  @param[in] value The high host flap threshold.
 */
void configuration::set_high_host_flap_threshold(float value) {
  if (value <= 0.0 || value >= 100.0) {
    throw (error() << "high_host_flap_threshold: invalid value.");
  }
  _tab_float[high_host_flap_threshold] = value;
}

/**
 *  Set the date format.
 *  @param[in] value The date format.
 */
void configuration::set_date_format(e_date_format value) {
  _tab_uint[date_format] = value;
}

/**
 *  Set the data format.
 *  @param[in] value The date format.
 */
void configuration::set_date_format(std::string const& value) {
  if (value == "euro") {
    _tab_uint[date_format] = euro;
  }
  else if (value == "iso8601") {
    _tab_uint[date_format] = iso8601;
  }
  else if (value == "strict-iso8601") {
    _tab_uint[date_format] = strict_iso8601;
  }
  else {
    _tab_uint[date_format] = us;
  }
}

/**
 *  Set the logging rotation method.
 *  @param[in] value The logging rotation method.
 */
void configuration::set_log_rotation_method(e_log_rotation value) {
  _tab_uint[log_rotation_method] = value;
}

/**
 *  Set the logging rotation method.
 *  @param[in] value The logging rotation method.
 */
void configuration::set_log_rotation_method(std::string const& value) {
  if (value == "n") {
    _tab_uint[log_rotation_method] = rot_none;
  }
  else if (value == "h") {
    _tab_uint[log_rotation_method] = rot_hourly;
  }
  else if (value == "d") {
    _tab_uint[log_rotation_method] = rot_daily;
  }
  else if (value == "w") {
    _tab_uint[log_rotation_method] = rot_weekly;
  }
  else if (value == "m") {
    _tab_uint[log_rotation_method] = rot_monthly;
  }
  else {
    throw (error() << "log_rotation_method: invalid value.");
  }
}

/**
 *  Set the service inter check delay method.
 *  @param[in] value The service inter check delay method.
 */
void configuration::set_service_inter_check_delay_method(e_inter_check_delay value) {
  _tab_uint[service_inter_check_delay_method] = value;
}

/**
 *  Set the service inter check delay method.
 *  @param[in] value The service inter check delay method.
 */
void configuration::set_service_inter_check_delay_method(std::string const& value) {
  if (value == "n") {
    _tab_uint[service_inter_check_delay_method] = icd_none;
  }
  else if (value == "d") {
    _tab_uint[service_inter_check_delay_method] = icd_dumb;
  }
  else if (value == "s") {
    _tab_uint[service_inter_check_delay_method] = icd_smart;
  }
  else {
    _tab_uint[service_inter_check_delay_method] = icd_user;
    if (_str2obj<double>(value, &scheduling_info.service_inter_check_delay) == false ||
	scheduling_info.service_inter_check_delay <= 0.0) {
      throw (error() << "service_inter_check_delay_method: invalid value.");
    }
  }
}

/**
 *  Set the host inter check delay method.
 *  @param[in] value The host inter check delay method.
 */
void configuration::set_host_inter_check_delay_method(e_inter_check_delay value) {
  _tab_uint[host_inter_check_delay_method] = value;
}

/**
 *  Set the host inter check delay method.
 *  @param[in] value The host inter check delay method.
 */
void configuration::set_host_inter_check_delay_method(std::string const& value) {
  if (value == "n") {
    _tab_uint[host_inter_check_delay_method] = icd_none;
  }
  else if (value == "d") {
    _tab_uint[host_inter_check_delay_method] = icd_dumb;
  }
  else if (value == "s") {
    _tab_uint[host_inter_check_delay_method] = icd_smart;
  }
  else {
    _tab_uint[host_inter_check_delay_method] = icd_user;
    if (_str2obj<double>(value, &scheduling_info.host_inter_check_delay) == false ||
	scheduling_info.host_inter_check_delay <= 0.0) {
      throw (error() << "host_inter_check_delay_method: invalid value.");
    }
  }
}

/**
 *  Set the service interleave factor method.
 *  @param[in] value The service interleave factor method.
 */
void configuration::set_service_interleave_factor_method(e_interleave_factor value) {
  _tab_uint[service_interleave_factor_method] = value;
}

/**
 *  Set the service interleave factor method.
 *  @param[in] value The service interleave factor method.
 */
void configuration::set_service_interleave_factor_method(std::string const& value) {
  if (value == "s") {
    _tab_uint[service_interleave_factor_method] = ilf_smart;
  }
  else {
    _tab_uint[service_interleave_factor_method] = ilf_user;
    if (_str2obj<int>(value, &scheduling_info.service_interleave_factor) == false ||
	scheduling_info.service_interleave_factor < 1) {
      scheduling_info.service_interleave_factor = 1;
    }
  }
}

/**************************************
 *                                     *
 *           Private Methods           *
 *                                     *
 **************************************/

/**
 *  Get the next entire line from a stream.
 *
 *  @param[in] ifs The file stream.
 *
 *  @return The line.
 */
std::string configuration::_getline(std::ifstream& ifs) throw() {
  std::string line;
  while (!ifs.eof()) {
    std::string buf;
    std::getline(ifs, buf);
    if (buf.empty()) {
      return ("");
    }
    if (buf.size() > 1 && buf[buf.size() - 2] != '\\' && buf[buf.size() - 1] == '\\') {
      line.append(buf.begin(), buf.end() - 2);
      continue;
    }
    else if (buf[buf.size() - 1] == '\\') {
      buf.erase(buf.size() - 1);
    }
    return (_trim(line).append(buf));
  }
  return ("");
}

/**
 *  Trim a string.
 *
 *  @param[in] str The string.
 *
 *  @return The trimming stream.
 */
std::string& configuration::_trim(std::string& str) throw() {
  const char* whitespaces = " \t\r\n";
  size_t pos = str.find_last_not_of(whitespaces);

  if (pos == std::string::npos)
    str.clear();
  else
    {
      str.erase(pos + 1);
      if ((pos = str.find_first_not_of(whitespaces)) != std::string::npos)
        str.erase(0, pos);
    }
  return (str);
}

/**
 *  Reset variable
 */
void configuration::_reset() {
  set_log_file(DEFAULT_LOG_FILE);
  set_temp_file(DEFAULT_TEMP_FILE);
  set_command_file(DEFAULT_COMMAND_FILE);
  set_p1_file(DEFAULT_P1_FILE);
  set_debug_file(DEFAULT_DEBUG_FILE);


  set_use_regexp_matches(DEFAULT_USE_REGEXP_MATCHES);
  set_use_true_regexp_matching(DEFAULT_USE_TRUE_REGEXP_MATCHING);

  set_use_syslog(DEFAULT_USE_SYSLOG);
  set_log_service_retries(DEFAULT_LOG_SERVICE_RETRIES);
  set_log_host_retries(DEFAULT_LOG_HOST_RETRIES);
  set_log_initial_states(DEFAULT_LOG_INITIAL_STATES);

  set_log_notifications(DEFAULT_NOTIFICATION_LOGGING);
  set_log_event_handlers(DEFAULT_LOG_EVENT_HANDLERS);
  set_log_external_commands(DEFAULT_LOG_EXTERNAL_COMMANDS);
  set_log_passive_checks(DEFAULT_LOG_PASSIVE_CHECKS);

  set_service_check_timeout(DEFAULT_SERVICE_CHECK_TIMEOUT);
  set_host_check_timeout(DEFAULT_HOST_CHECK_TIMEOUT);
  set_event_handler_timeout(DEFAULT_EVENT_HANDLER_TIMEOUT);
  set_notification_timeout(DEFAULT_NOTIFICATION_TIMEOUT);
  set_ocsp_timeout(DEFAULT_OCSP_TIMEOUT);
  set_ochp_timeout(DEFAULT_OCHP_TIMEOUT);

  set_sleep_time(DEFAULT_SLEEP_TIME);
  _tab_uint[interval_length] = DEFAULT_INTERVAL_LENGTH;
  set_service_inter_check_delay_method(DEFAULT_SERVICE_INTER_CHECK_DELAY_METHOD);
  set_host_inter_check_delay_method(DEFAULT_HOST_INTER_CHECK_DELAY_METHOD);
  set_service_interleave_factor_method(DEFAULT_SERVICE_INTERLEAVE_FACTOR_METHOD);
  set_max_service_check_spread(DEFAULT_SERVICE_CHECK_SPREAD);
  set_max_host_check_spread(DEFAULT_HOST_CHECK_SPREAD);

  set_use_aggressive_host_checking(DEFAULT_AGGRESSIVE_HOST_CHECKING);
  set_cached_host_check_horizon(DEFAULT_CACHED_HOST_CHECK_HORIZON);
  set_cached_service_check_horizon(DEFAULT_CACHED_SERVICE_CHECK_HORIZON);
  set_enable_predictive_host_dependency_checks(DEFAULT_ENABLE_PREDICTIVE_HOST_DEPENDENCY_CHECKS);
  set_enable_predictive_service_dependency_checks(DEFAULT_ENABLE_PREDICTIVE_SERVICE_DEPENDENCY_CHECKS);

  set_command_check_interval(DEFAULT_COMMAND_CHECK_INTERVAL);
  set_check_reaper_interval(DEFAULT_CHECK_REAPER_INTERVAL);
  set_max_check_reaper_time(DEFAULT_MAX_REAPER_TIME);
  set_max_check_result_file_age(DEFAULT_MAX_CHECK_RESULT_AGE);
  set_service_freshness_check_interval(DEFAULT_FRESHNESS_CHECK_INTERVAL);
  set_host_freshness_check_interval(DEFAULT_FRESHNESS_CHECK_INTERVAL);
  set_auto_rescheduling_interval(DEFAULT_AUTO_RESCHEDULING_INTERVAL);
  set_auto_rescheduling_window(DEFAULT_AUTO_RESCHEDULING_WINDOW);

  set_check_external_commands(DEFAULT_CHECK_EXTERNAL_COMMANDS);
  set_check_orphaned_services(DEFAULT_CHECK_ORPHANED_SERVICES);
  set_check_orphaned_hosts(DEFAULT_CHECK_ORPHANED_HOSTS);
  set_check_service_freshness(DEFAULT_CHECK_SERVICE_FRESHNESS);
  set_check_host_freshness(DEFAULT_CHECK_HOST_FRESHNESS);
  set_auto_reschedule_checks(DEFAULT_AUTO_RESCHEDULE_CHECKS);

  set_soft_state_dependencies(DEFAULT_SOFT_STATE_DEPENDENCIES);

  set_retain_state_information(DEFAULT_RETAIN_STATE_INFORMATION);
  set_retention_update_interval(DEFAULT_RETENTION_UPDATE_INTERVAL);
  set_use_retained_program_state(DEFAULT_USE_RETAINED_PROGRAM_STATE);
  set_use_retained_scheduling_info(DEFAULT_USE_RETAINED_SCHEDULING_INFO);
  set_retention_scheduling_horizon(DEFAULT_RETENTION_SCHEDULING_HORIZON);
  set_retained_host_attribute_mask(DEFAULT_RETAINED_HOST_ATTRIBUTE_MASK);
  set_retained_process_host_attribute_mask(DEFAULT_RETAINED_PROCESS_HOST_ATTRIBUTE_MASK);
  set_retained_contact_host_attribute_mask(DEFAULT_RETAINED_CONTACT_HOST_ATTRIBUTE_MASK);
  set_retained_contact_service_attribute_mask(DEFAULT_RETAINED_CONTACT_SERVICE_ATTRIBUTE_MASK);

  set_log_rotation_method(DEFAULT_LOG_ROTATION);

  set_max_parallel_service_checks(DEFAULT_MAX_PARALLEL_SERVICE_CHECKS);

  set_enable_notifications(DEFAULT_ENABLE_NOTIFICATIONS);
  set_execute_service_checks(DEFAULT_EXECUTE_SERVICE_CHECKS);
  set_accept_passive_service_checks(DEFAULT_ACCEPT_PASSIVE_SERVICE_CHECKS);
  set_execute_host_checks(DEFAULT_EXECUTE_HOST_CHECKS);
  set_accept_passive_service_checks(DEFAULT_ACCEPT_PASSIVE_SERVICE_CHECKS);
  set_enable_event_handlers(DEFAULT_ENABLE_EVENT_HANDLERS);
  set_obsess_over_services(DEFAULT_OBSESS_OVER_SERVICES);
  set_obsess_over_hosts(DEFAULT_OBSESS_OVER_HOSTS);
  set_enable_failure_prediction(DEFAULT_ENABLE_FAILURE_PREDICTION);

  set_status_update_interval(DEFAULT_STATUS_UPDATE_INTERVAL);

  set_event_broker_options(DEFAULT_EVENT_BROKER_OPTIONS);

  set_time_change_threshold(DEFAULT_TIME_CHANGE_THRESHOLD);

  set_enable_flap_detection(DEFAULT_ENABLE_FLAP_DETECTION);
  set_low_service_flap_threshold(DEFAULT_LOW_SERVICE_FLAP_THRESHOLD);
  set_high_service_flap_threshold(DEFAULT_HIGH_SERVICE_FLAP_THRESHOLD);
  set_low_host_flap_threshold(DEFAULT_LOW_HOST_FLAP_THRESHOLD);
  set_high_host_flap_threshold(DEFAULT_HIGH_HOST_FLAP_THRESHOLD);

  set_process_performance_data(DEFAULT_PROCESS_PERFORMANCE_DATA);

  set_translate_passive_host_checks(DEFAULT_TRANSLATE_PASSIVE_HOST_CHECKS);
  set_passive_host_checks_are_soft(DEFAULT_PASSIVE_HOST_CHECKS_SOFT);

  set_use_large_installation_tweaks(DEFAULT_USE_LARGE_INSTALLATION_TWEAKS);
  set_enable_environment_macros(DEFAULT_ENABLE_ENVIRONMENT_MACROS);
  set_free_child_process_memory(DEFAULT_FREE_CHILD_PROCESS_MEMORY);
  _tab_int[child_processes_fork_twice] = DEFAULT_CHILD_PROCESSES_FORK_TWICE;

  set_additional_freshness_latency(DEFAULT_ADDITIONAL_FRESHNESS_LATENCY);

  set_enable_embedded_perl(DEFAULT_ENABLE_EMBEDDED_PERL);
  set_use_embedded_perl_implicitly(DEFAULT_USE_EMBEDDED_PERL_IMPLICITLY);

  set_external_command_buffer_slots(DEFAULT_EXTERNAL_COMMAND_BUFFER_SLOTS);

  set_debug_level(DEFAULT_DEBUG_LEVEL);
  set_debug_verbosity(DEFAULT_DEBUG_VERBOSITY);
  set_max_debug_file_size(DEFAULT_MAX_DEBUG_FILE_SIZE);

  set_date_format(DEFAULT_DATE_FORMAT);

  set_global_host_event_handler("");
  set_global_service_event_handler("");

  set_ocsp_command("");
  set_ochp_command("");
}

/**
 *  Parse the resource file.
 *  @param[in] value The filename.
 */
void configuration::_parse_resource_file(std::string const& value) {
  std::ifstream ifs;
  ifs.open(value.c_str());
  if (ifs.fail()) {
    throw (error() << "cannot open resource file: `" << value << "'");
  }

  unsigned int save_cur_line = _cur_line;
  std::string save_filename = _filename;
  _filename = value;

  for (_cur_line = 1; !ifs.eof(); ++_cur_line) {
    std::string line = _getline(ifs);
    if (line == "" || line[0] == '#') {
      continue;
    }

    size_t pos = line.find_first_of('=');
    if (pos == std::string::npos) {
      throw (error() << "[" << _filename << ":" << _cur_line
	     << "] bad variable name: `" << line << "'");
    }
    std::string key = line.substr(0, pos);
    unsigned int user_index;
    if (!_trim(key).compare(0, 5, "$USER") && key[key.size() - 1] == '$') {
      key = key.substr(5, key.size() - 6);
      if (_str2obj<unsigned int>(key, &user_index) == false ||
	  user_index >= MAX_USER_MACROS) {
	logit(NSLOG_CONFIG_WARNING, TRUE, "Warning: [%s:%d] bad variable name `%s'",
	      _filename.c_str(), _cur_line, key.c_str());
	continue;
      }

      std::string value = line.substr(pos + 1);
      delete[] macro_user[user_index];
      macro_user[user_index] = my_strdup(_trim(value).c_str());
    }
    else {
      logit(NSLOG_CONFIG_WARNING, TRUE, "Warning: [%s:%d] bad variable name `%s'",
       	    _filename.c_str(), _cur_line, key.c_str());
    }
  }

  _cur_line = save_cur_line;
  _filename = save_filename;

  ifs.close();

  delete[] _mac->x[MACRO_RESOURCEFILE];
  _mac->x[MACRO_RESOURCEFILE] = my_strdup(value.c_str());
}

/**
 *  Set the auth filename.
 *  @param[in] value The filename.
 */
void configuration::_set_auth_file(std::string const& value) {
  (void)value;
  logit(NSLOG_CONFIG_WARNING, TRUE, "Warning: auth_file variable ignored.");
}

/**
 *  Set the admin email macro.
 *  @param[in] value The admin email.
 */
void configuration::_set_admin_email(std::string const& value) {
  delete[] _mac->x[MACRO_ADMINEMAIL];
  _mac->x[MACRO_ADMINEMAIL] = my_strdup(value.c_str());
}

/**
 *  Set the admin pager macro.
 *  @param[in] value The admin pager.
 */
void configuration::_set_admin_pager(std::string const& value) {
  delete[] _mac->x[MACRO_ADMINPAGER];
  _mac->x[MACRO_ADMINPAGER] = my_strdup(value.c_str());
}

/**
 *  Retained sercice attribute mask ignored.
 */
void configuration::_set_retained_service_attribute_mask(std::string const& value) {
  (void)value;
  logit(NSLOG_CONFIG_WARNING, TRUE, "Warning: retained_service_attribute_mask variable ignored.");
}

/**
 * Retained process service attribute mask ignored.
 */
void configuration::_set_retained_process_service_attribute_mask(std::string const& value) {
  (void)value;
  logit(NSLOG_CONFIG_WARNING, TRUE, "Warning: retained_process_service_attribute_mask variable ignored.");
}

/**
 *  Aggrefate status updates ignored.
 */
void configuration::_set_aggregate_status_updates(std::string const& value) {
  (void)value;
  // DEPRECATED
  logit(NSLOG_CONFIG_WARNING, TRUE, "Warning: aggregate_status_updates directive ignored.  All status file updates are now aggregated.");
}

/**
 *  Set the broker module.
 *  @param[in] value The broker module.
 */
void configuration::_set_broker_module(std::string const& value) {
  size_t pos = value.find_first_of(" \n");
  if (pos == std::string::npos)
    throw (error() << "broker_module: invalid value.");

#ifdef USE_EVENT_BROKER
  std::string mod = value.substr(0, pos);
  std::string arg = value.substr(pos + 1, value.find_first_of(" \n", pos + 1) - pos);

  neb_add_module(mod.c_str(), arg.c_str(), TRUE);
#endif
}

/**
 *  Bare update check ignored.
 */
void configuration::_set_bare_update_check(std::string const& value) {
  (void)value;
  logit(NSLOG_CONFIG_WARNING, TRUE, "Warning: bare_update_check variable ignored. Centreon Scheduler does not check for updates.");
}

/**
 *  Check for updates ignored.
 */
void configuration::_set_check_for_updates(std::string const& value) {
  (void)value;
  logit(NSLOG_CONFIG_WARNING, TRUE, "Warning: check_for_updates variable ignored. Centreon Scheduler does not check for updates.");
}

/**
 * Comment file ignored.
 */
void configuration::_set_comment_file(std::string const& value) {
  (void)value;
  logit(NSLOG_CONFIG_WARNING, TRUE, "Warning: comment_file variable ignored. Comments are now stored in the status and retention files.");
}

/**
 * Daemon dumps core ignored.
 */
void configuration::_set_daemon_dumps_core(std::string const& value) {
  (void)value;
  logit(NSLOG_CONFIG_WARNING, TRUE, "Warning: daemon_dumps_core variable ignored. Core dumping has to be handled by Centreon Scheduler user.");
}

/**
 * Downtime file ignored.
 */
void configuration::_set_downtime_file(std::string const& value) {
  (void)value;
  logit(NSLOG_CONFIG_WARNING, TRUE, "Warning: downtime_file variable ignored. Downtime entries are now stored in the status and retention files.");
}

/**
 * Lock file ignored.
 */
void configuration::_set_lock_file(std::string const& value) {
  (void)value;
  logit(NSLOG_CONFIG_WARNING, TRUE, "Warning: lock_file variable ignored. Priviledge drop should be handled by startup script.");
}

/**
 * Nagios user ignored.
 */
void configuration::_set_nagios_user(std::string const& value) {
  (void)value;
  logit(NSLOG_CONFIG_WARNING, TRUE, "Warning: nagios_user varible ignored. Priviledge drop should be handled by startup script.");
}

/**
 * Nagios group ignored.
 */
void configuration::_set_nagios_group(std::string const& value) {
  (void)value;
  logit(NSLOG_CONFIG_WARNING, TRUE, "Warning: nagios_group variable ignored. Priviledge drop should be handled by startup script.");
}
