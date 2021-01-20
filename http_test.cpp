/**
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
 **/
 
#include "httplib.h"
#include <chrono>
#include <ctime>
#include <iostream>
#include <libpq-fe.h>

void tx(const httplib::Request &req, httplib::Response &res);
void logger(const httplib::Request &req, const httplib::Response &res);
std::string get_configuration(const std::string &env_name,
                              const std::string &default_value = "");

/**
 * The database URL to be used as a libpq connection string
 */
const std::string database_url = get_configuration("DATABASE_URL");
const std::string sql_query = get_configuration("SQL_QUERY", "SELECT 1");

/**
 * Everything starts from here
 */
int main() {
  // Initial logging
  std::cout << "Database URL: " << std::quoted(database_url) << std::endl;
  std::cout << "SQL query: " << std::quoted(sql_query) << std::endl;
  std::cout.flush();

  // Starting HTTP server
  httplib::Server svr;
  svr.set_logger(logger);
  svr.Get("/tx", tx);
  svr.listen("0.0.0.0", 8080);

  return 0;
}

/**
 * Given a request/response pair, put a small log message in stdout
 */
void logger(const httplib::Request &req, const httplib::Response &res) {
  using namespace std;

  const auto now = std::chrono::system_clock::now();
  const auto nowAsTimeT = std::chrono::system_clock::to_time_t(now);
  const auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                         now.time_since_epoch()) %
                     1000;

  cout << std::put_time(std::localtime(&nowAsTimeT), "%a %b %d %Y %T") << '.'
       << std::setfill('0') << std::setw(3) << nowMs.count();
  cout << " " << req.remote_addr << " " << req.path << " - " << res.status
       << endl;
  cout.flush();
}

/**
 * The test transaction on PostgreSQL
 */
void tx(const httplib::Request &req, httplib::Response &res) {
  PGconn *conn = PQconnectdb(database_url.c_str());

  if (PQstatus(conn) != CONNECTION_OK) {
    res.set_content("Connection error", "text/plain");
    res.status = 500;
    std::cout << PQerrorMessage(conn) << std::endl;
    std::cout.flush();
  } else {
    PGresult *pgres = PQexec(conn, sql_query.c_str());
    if (PQresultStatus(pgres) != PGRES_COMMAND_OK &&
        PQresultStatus(pgres) != PGRES_TUPLES_OK) {
      res.set_content("Query error", "text/plain");
      res.status = 500;
      std::cout << PQresultErrorMessage(pgres) << std::endl;
      std::cout.flush();
    } else {
      res.set_content("Ok!", "text/plain");
    }
    PQclear(pgres);
  }

  PQfinish(conn);
}

/**
 * Get a configuration parameter or returns the default value
 */
std::string get_configuration(const std::string &env_name,
                              const std::string &default_value) {
  auto value = std::getenv(env_name.c_str());
  if (value == NULL) {
    return default_value;
  } else {
    return value;
  }
}