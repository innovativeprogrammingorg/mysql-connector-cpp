/*
   Copyright 2008 - 2009 Sun Microsystems, Inc.

   The MySQL Connector/C++ is licensed under the terms of the GPL
   <http://www.gnu.org/licenses/old-licenses/gpl-2.0.html>, like most
   MySQL Connectors. There are special exceptions to the terms and
   conditions of the GPL as it is applied to this software, see the
   FLOSS License Exception
   <http://www.mysql.com/about/legal/licensing/foss-exception.html>.
 */

#include <cppconn/warning.h>

#include "connection.h"
#include <stdlib.h>
#include <cppconn/connection.h>

namespace testsuite
{
namespace classes
{

void connection::getClientInfo()
{
  logMsg("connection::getClientInfo() - MySQL_Connection::getClientInfo()");
  try
  {
    std::string client_info;
    std::string ret;

    ret=con->getClientInfo(client_info);
    if (ret != "cppconn")
      FAIL("Expecting 'cppconn' got '" + ret + "'.");

    if (!client_info.empty())
      FAIL("Expecting parameter to be unchanged but it seems to have been modified.");

  }
  catch (sql::SQLException &e)
  {
    logErr(e.what());
    logErr("SQLState: " + e.getSQLState());
    fail(e.what(), __FILE__, __LINE__);
  }
}

void connection::getNoWarningsOnNewLine()
{
  logMsg("connection::getNoWarningsOnNewLine() - MySQL_Connection::getWarnings()");
  try
  {
    const sql::SQLWarning* warning;

    warning=con->getWarnings();
    if (warning != NULL)
      FAIL("There should be no warnings on the default connection");

  }
  catch (sql::SQLException &e)
  {
    logErr(e.what());
    logErr("SQLState: " + e.getSQLState());
    fail(e.what(), __FILE__, __LINE__);
  }
}

void connection::getNoWarningsAfterClear()
{
  logMsg("connection::getNoWarningsAfterClear() - MySQL_Connection::getWarnings()");
  try
  {
    const sql::SQLWarning* warning;

    /* TODO: pointless test as there is no warning before running clearWarnings() */
    con->clearWarnings();
    warning=con->getWarnings();
    if (warning != NULL)
      FAIL("There should be no warnings on the default connection");

  }
  catch (sql::SQLException &e)
  {
    logErr(e.what());
    logErr("SQLState: " + e.getSQLState());
    fail(e.what(), __FILE__, __LINE__);
  }
}

void connection::checkClosed()
{
  logMsg("connection::checkClosed - MySQL_Connection::close, isClosed() and internal check_closed()");
  try
  {
    if (con->isClosed())
      FAIL("Connection should not be reported as closed");

    con->close();

    if (!con->isClosed())
      FAIL("Connection should be closed");

    try
    {
      con->rollback();
    }
    catch (sql::SQLException &e)
    {
      std::string reason(exceptionIsOK(e, "HY000", 1000));
      if (!reason.empty())
        FAIL(reason);
    }


  }
  catch (sql::SQLException &e)
  {
    logErr(e.what());
    logErr("SQLState: " + e.getSQLState());
    fail(e.what(), __FILE__, __LINE__);
  }
}

void connection::connectUsingMap()
{
  logMsg("connection::connectUsingMap - using map to pass connection parameters");

  try
  {
    std::map<std::string, sql::ConnectPropertyVal> connection_properties;

    {
      sql::ConnectPropertyVal tmp;
      /* url comes from the unit testing framework */
      tmp.str.val=url.c_str();
      tmp.str.len=url.length();
      connection_properties[std::string("hostName")]=tmp;
    }

    {
      sql::ConnectPropertyVal tmp;
      /* user comes from the unit testing framework */
      tmp.str.val=user.c_str();
      tmp.str.len=user.length();
      connection_properties[std::string("userName")]=tmp;
    }

    {
      sql::ConnectPropertyVal tmp;
      tmp.str.val=passwd.c_str();
      tmp.str.len=passwd.length();
      connection_properties[std::string("password")]=tmp;
    }

    created_objects.clear();
    con.reset(driver->connect(connection_properties));

    /*
    The property map now contains the minimum required entries and it works.
    Lets play with so-to-say optional entries.
     */

    /* 1) Port */
    connection_properties.erase("port");
    {
      int port= -1;
      if (url.compare(0, sizeof ("tcp://") - 1, "tcp://") == 0)
      {
        size_t port_pos;
        port_pos=url.find_last_of(":", std::string::npos);
        if (port_pos != std::string::npos)
          port=atoi(url.substr(port_pos + 1, std::string::npos).c_str());

        if (port == -1)
        {
          /* The user is using TCP/IP and default port 3306 */
          sql::ConnectPropertyVal tmp;
          tmp.lval=(long long) port;
          try
          {
            created_objects.clear();
            con.reset(driver->connect(connection_properties));
            FAIL("Connect works with invalid port of -1");
          }
          catch (sql::SQLException &e)
          {
            std::string reason(exceptionIsOK(e, "HY000", 2003));
            logErr(reason);
          }
        }
        else
        {
          /* The user is using TCP/IP and has specified the port.
           A port setting shall NOT overrule the setting from the URL */
          port= -1;
          sql::ConnectPropertyVal tmp;
          tmp.lval=(long long) port;
          try
          {
            created_objects.clear();
            con.reset(driver->connect(connection_properties));
          }
          catch (sql::SQLException &)
          {
            FAIL("URL shall overrule port setting");
          }
        }

      }
      else
      {
        /* We must be using a socket connection - all port settings shall be ignored */
        sql::ConnectPropertyVal tmp;
        tmp.lval=(long long) port;
        connection_properties[std::string("port")]=tmp;
        /* Must not throw an exception */
        try
        {
          created_objects.clear();
          con.reset(driver->connect(connection_properties));
        }
        catch (sql::SQLException &)
        {
          FAIL("Port setting should be ignored, using Unix socket!");
        }
      }
    }
    connection_properties.erase("port");

    /* 2) Socket */
    connection_properties.erase("socket");
    {
      std::string socket("");
#ifndef CPPWIN_WIN32

      if (url.compare(0, sizeof ("unix://") - 1, "unix://") == 0)
      {
        // Unix socket connection
        socket="I hope this is invalid";
        try
        {
          created_objects.clear();
          con.reset(driver->connect(connection_properties));
        }
        catch (sql::SQLException &)
        {
          FAIL("Socket setting should be ignored, socket is part of the URL");
        }

      }
      else
#endif
        if (url.compare(0, sizeof ("tcp://") - 1, "tcp://") == 0)
      {
        // TCP/IP connection, socket shall be ignored
        socket="I hope this is invalid";
        try
        {
          created_objects.clear();
          con.reset(driver->connect(connection_properties));
        }
        catch (sql::SQLException &)
        {
          FAIL("Socket setting should be ignored because its a TCP/IP connection");
        }
      }
    }
    connection_properties.erase("socket");

    /* 2) Schema */
    connection_properties.erase("schema");
    {
      std::string schema("");
      std::string myschema("mysql");
      std::string retschema("");

      if (url.compare(0, sizeof ("tcp://") - 1, "tcp://") == 0)
      {
        // TCP/IP connection - schema cannot be set when using unix socket syntax
        size_t schema_pos;
        std::string host(url.substr(sizeof ("tcp://") - 1, std::string::npos));
        schema_pos=host.find("/");
        if (schema_pos != std::string::npos)
        {
          schema_pos++;
          schema=host.substr(schema_pos, host.size() - schema_pos);
        }
      }

      if (schema.empty())
      {
        logMsg("... schema not set through the URL");

        sql::ConnectPropertyVal tmp;
        tmp.str.val=schema.c_str();
        tmp.str.len=schema.length();
        connection_properties[std::string("schema")]=tmp;

        try
        {
          created_objects.clear();
          con.reset(driver->connect(connection_properties));
          schema=con->getSchema();
          if (!schema.empty())
            FAIL("Empty schama specified but certain schema selected upon connect");
        }
        catch (sql::SQLException &)
        {
          FAIL("Connect should have worked although schema property set to empty string");
        }


        logMsg("... trying to connect to mysql schema, may or may not work");

        connection_properties.erase("schema");
        tmp.str.val=myschema.c_str();
        tmp.str.len=myschema.length();
        connection_properties[std::string("schema")]=tmp;

        try
        {
          created_objects.clear();
          con.reset(driver->connect(connection_properties));
          Connection mycon(driver->connect(connection_properties));
          retschema=mycon->getSchema();
          if (retschema != myschema)
          {
            logErr(retschema);
            logErr(myschema);
            logErr(mycon->getCatalog());
            logErr(mycon->getSchema());
            FAIL("Connected to schema mysql but getSchema() reports different schema");
          }
        }
        catch (sql::SQLException &)
        {
          logMsg("... cannot connect to mysql schema but that is OK, might be insufficient grants");
        }

      }
      else
      {
        /* schema is set in the TCP/IP url */
        logMsg("... schema is set in the URL and property shall be ignored");

        /* no property set */
        try
        {
          created_objects.clear();
          con.reset(driver->connect(connection_properties));
          retschema=con->getSchema();
          if (retschema != schema)
          {
            logErr(retschema);
            logErr(schema);
            FAIL("Connected to a certain schema but getSchema() reports different schema");
          }
        }
        catch (sql::SQLException &)
        {
          FAIL("Connect should not fail");
        }

        /* property set */
        sql::ConnectPropertyVal tmp;
        tmp.str.val=myschema.c_str();
        tmp.str.len=myschema.length();
        connection_properties[std::string("schema")]=tmp;
        try
        {
          created_objects.clear();
          con.reset(driver->connect(connection_properties));
          retschema=con->getSchema();
          if (retschema != schema)
          {
            logErr(retschema);
            logErr(schema);
            FAIL("Connected to a certain schema but getSchema() reports different schema");
          }
        }
        catch (sql::SQLException &)
        {
          FAIL("Connect should not fail");
        }
      }
    }
    connection_properties.erase("schema");

    /* 3) ssl* */
    connection_properties.erase("sslKey");
    connection_properties.erase("sslCert");
    connection_properties.erase("sslCA");
    connection_properties.erase("sslCAPath");
    connection_properties.erase("sslCipher");
    {
      logMsg("... setting bogus SSL properties");

      std::string sql("ramdom bogus value");
      sql::ConnectPropertyVal tmp;
      tmp.str.val=sql.c_str();
      tmp.str.len=sql.length();
      connection_properties[std::string("sslKey")]=tmp;
      connection_properties[std::string("sslCert")]=tmp;
      connection_properties[std::string("sslCA")]=tmp;
      connection_properties[std::string("sslCAPath")]=tmp;
      connection_properties[std::string("sslCipher")]=tmp;
      /*
       mysql_ssl_set is silly:
       This function always returns 0.
       If SSL setup is incorrect, mysql_real_connect()
       returns an error when you attempt to connect.
       */
      try
      {
        created_objects.clear();
        con.reset(driver->connect(connection_properties));
      }
      catch (sql::SQLException &)
      {
      }

    }
    connection_properties.erase("sslKey");
    connection_properties.erase("sslCert");
    connection_properties.erase("sslCA");
    connection_properties.erase("sslCAPath");
    connection_properties.erase("sslCipher");

    /* All the CLIENT* are pointless. There is no way (yet) to verify the settings */
    /* 4) CLIENT_COMPRESS */
    connection_properties.erase("CLIENT_COMPRESS");
    {
      logMsg("... testing CLIENT_COMPRESS");
      sql::ConnectPropertyVal tmp;
      tmp.bval=true;
      connection_properties[std::string("CLIENT_COMPRESS")]=tmp;
      try
      {
        created_objects.clear();
        con.reset(driver->connect(connection_properties));
      }
      catch (sql::SQLException &)
      {
      }

      connection_properties[std::string("CLIENT_COMPRESS")]=tmp;
      try
      {
        created_objects.clear();
        con.reset(driver->connect(connection_properties));
      }
      catch (sql::SQLException &)
      {
      }

      connection_properties.erase("CLIENT_COMPRESS");
      tmp.lval=(long long) false;
      connection_properties[std::string("CLIENT_COMPRESS")]=tmp;
      try
      {
        created_objects.clear();
        con.reset(driver->connect(connection_properties));
      }
      catch (sql::SQLException &)
      {
      }
    }
    connection_properties.erase("CLIENT_COMPRESS");

    /* 5) CLIENT_FOUND_ROWS */
    connection_properties.erase("CLIENT_FOUND_ROWS");
    {
      logMsg("... testing CLIENT_FOUND_ROWS");
      sql::ConnectPropertyVal tmp;
      tmp.bval=true;
      connection_properties[std::string("CLIENT_FOUND_ROWS")]=tmp;
      try
      {
        created_objects.clear();
        con.reset(driver->connect(connection_properties));
      }
      catch (sql::SQLException &)
      {
      }

      connection_properties[std::string("CLIENT_FOUND_ROWS")]=tmp;
      try
      {
        created_objects.clear();
        con.reset(driver->connect(connection_properties));
      }
      catch (sql::SQLException &)
      {
      }

      connection_properties.erase("CLIENT_FOUND_ROWS");
      tmp.lval=(long long) false;
      connection_properties[std::string("CLIENT_FOUND_ROWS")]=tmp;
      try
      {
        created_objects.clear();
        con.reset(driver->connect(connection_properties));
      }
      catch (sql::SQLException &)
      {
      }
    }
    connection_properties.erase("CLIENT_FOUND_ROWS");

    /* 6) CLIENT_IGNORE_SIGPIPE */
    connection_properties.erase("CLIENT_IGNORE_SIGPIPE");
    {
      logMsg("... testing CLIENT_IGNORE_SIGPIPE");
      sql::ConnectPropertyVal tmp;
      tmp.bval=true;
      connection_properties[std::string("CLIENT_IGNORE_SIGPIPE")]=tmp;
      try
      {
        created_objects.clear();
        con.reset(driver->connect(connection_properties));
      }
      catch (sql::SQLException &)
      {
      }

      connection_properties[std::string("CLIENT_IGNORE_SIGPIPE")]=tmp;
      try
      {
        created_objects.clear();
        con.reset(driver->connect(connection_properties));
      }
      catch (sql::SQLException &)
      {
      }

      connection_properties.erase("CLIENT_IGNORE_SIGPIPE");
      tmp.lval=(long long) false;
      connection_properties[std::string("CLIENT_IGNORE_SIGPIPE")]=tmp;
      try
      {
        created_objects.clear();
        con.reset(driver->connect(connection_properties));
      }
      catch (sql::SQLException &)
      {
      }
    }
    connection_properties.erase("CLIENT_IGNORE_SIGPIPE");

    /* 7) CLIENT_IGNORE_SPACE */
    connection_properties.erase("CLIENT_IGNORE_SPACE");
    {
      logMsg("... testing CLIENT_IGNORE_SPACE");
      sql::ConnectPropertyVal tmp;
      tmp.bval=true;
      connection_properties[std::string("CLIENT_IGNORE_SPACE")]=tmp;
      try
      {
        created_objects.clear();
        con.reset(driver->connect(connection_properties));
      }
      catch (sql::SQLException &)
      {
      }

      connection_properties[std::string("CLIENT_IGNORE_SPACE")]=tmp;
      try
      {
        created_objects.clear();
        con.reset(driver->connect(connection_properties));
      }
      catch (sql::SQLException &)
      {
      }

      connection_properties.erase("CLIENT_IGNORE_SPACE");
      tmp.lval=(long long) false;
      connection_properties[std::string("CLIENT_IGNORE_SPACE")]=tmp;
      try
      {
        created_objects.clear();
        con.reset(driver->connect(connection_properties));
      }
      catch (sql::SQLException &)
      {
      }
    }
    connection_properties.erase("CLIENT_IGNORE_SPACE");

    /* 8) CLIENT_INTERACTIVE */
    connection_properties.erase("CLIENT_INTERACTIVE");
    {
      logMsg("... testing CLIENT_INTERACTIVE");
      sql::ConnectPropertyVal tmp;
      tmp.bval=true;
      connection_properties[std::string("CLIENT_INTERACTIVE")]=tmp;
      try
      {
        created_objects.clear();
        con.reset(driver->connect(connection_properties));
      }
      catch (sql::SQLException &)
      {
      }

      connection_properties[std::string("CLIENT_INTERACTIVE")]=tmp;
      try
      {
        created_objects.clear();
        con.reset(driver->connect(connection_properties));
      }
      catch (sql::SQLException &)
      {
      }

      connection_properties.erase("CLIENT_INTERACTIVE");
      tmp.lval=(long long) false;
      connection_properties[std::string("CLIENT_INTERACTIVE")]=tmp;
      try
      {
        created_objects.clear();
        con.reset(driver->connect(connection_properties));
      }
      catch (sql::SQLException &)
      {
      }
    }
    connection_properties.erase("CLIENT_INTERACTIVE");

    /* 9) CLIENT_LOCAL_FILES */
    /* TODO - add proper test */
    connection_properties.erase("CLIENT_LOCAL_FILES");
    {
      logMsg("... testing CLIENT_LOCAL_FILES");
      sql::ConnectPropertyVal tmp;
      tmp.bval=true;
      connection_properties[std::string("CLIENT_LOCAL_FILES")]=tmp;
      try
      {
        created_objects.clear();
        con.reset(driver->connect(connection_properties));
      }
      catch (sql::SQLException &)
      {
      }

      connection_properties[std::string("CLIENT_LOCAL_FILES")]=tmp;
      try
      {
        created_objects.clear();
        con.reset(driver->connect(connection_properties));
      }
      catch (sql::SQLException &)
      {
      }

      connection_properties.erase("CLIENT_LOCAL_FILES");
      tmp.lval=(long long) false;
      connection_properties[std::string("CLIENT_LOCAL_FILES")]=tmp;
      try
      {
        created_objects.clear();
        con.reset(driver->connect(connection_properties));
      }
      catch (sql::SQLException &)
      {
      }
    }
    connection_properties.erase("CLIENT_LOCAL_FILES");

    /* 10) CLIENT_MULTI_RESULTS */
    /* TODO - add proper test */
    connection_properties.erase("CLIENT_MULTI_RESULTS");
    {
      logMsg("... testing CLIENT_MULTI_RESULTS");
      sql::ConnectPropertyVal tmp;
      tmp.bval=true;
      connection_properties[std::string("CLIENT_MULTI_RESULTS")]=tmp;
      try
      {
        created_objects.clear();
        con.reset(driver->connect(connection_properties));
      }
      catch (sql::SQLException &)
      {
      }

      connection_properties[std::string("CLIENT_MULTI_RESULTS")]=tmp;
      try
      {
        created_objects.clear();
        con.reset(driver->connect(connection_properties));
      }
      catch (sql::SQLException &)
      {
      }

      connection_properties.erase("CLIENT_MULTI_RESULTS");
      tmp.lval=(long long) false;
      connection_properties[std::string("CLIENT_MULTI_RESULTS")]=tmp;
      try
      {
        created_objects.clear();
        con.reset(driver->connect(connection_properties));
      }
      catch (sql::SQLException &)
      {
      }
    }
    connection_properties.erase("CLIENT_MULTI_RESULTS");

    /* 11) CLIENT_MULTI_STATEMENTS */
    /* TODO: add proper test */
    connection_properties.erase("CLIENT_MULTI_STATEMENTS");
    {
      logMsg("... testing CLIENT_MULTI_STATEMENTS");
      sql::ConnectPropertyVal tmp;
      tmp.bval=true;
      connection_properties[std::string("CLIENT_MULTI_STATEMENTS")]=tmp;
      try
      {
        created_objects.clear();
        con.reset(driver->connect(connection_properties));
      }
      catch (sql::SQLException &)
      {
      }

      connection_properties[std::string("CLIENT_MULTI_STATEMENTS")]=tmp;
      try
      {
        created_objects.clear();
        con.reset(driver->connect(connection_properties));
      }
      catch (sql::SQLException &)
      {
      }

      connection_properties.erase("CLIENT_MULTI_STATEMENTS");
      tmp.lval=(long long) false;
      connection_properties[std::string("CLIENT_MULTI_STATEMENTS")]=tmp;
      try
      {
        created_objects.clear();
        con.reset(driver->connect(connection_properties));
      }
      catch (sql::SQLException &)
      {
      }
    }
    connection_properties.erase("CLIENT_MULTI_STATEMENTS");

    /* 12) CLIENT_NO_SCHEMA */
    connection_properties.erase("CLIENT_NO_SCHEMA");
    {
      logMsg("... testing CLIENT_NO_SCHEMA");
      sql::ConnectPropertyVal tmp;
      tmp.bval=true;
      connection_properties[std::string("CLIENT_NO_SCHEMA")]=tmp;
      try
      {
        created_objects.clear();
        con.reset(driver->connect(connection_properties));
      }
      catch (sql::SQLException &)
      {
      }

      connection_properties[std::string("CLIENT_NO_SCHEMA")]=tmp;
      try
      {
        created_objects.clear();
        con.reset(driver->connect(connection_properties));
      }
      catch (sql::SQLException &)
      {
      }

      connection_properties.erase("CLIENT_NO_SCHEMA");
      tmp.lval=(long long) false;
      connection_properties[std::string("CLIENT_NO_SCHEMA")]=tmp;
      try
      {
        created_objects.clear();
        con.reset(driver->connect(connection_properties));
      }
      catch (sql::SQLException &)
      {
      }
    }
    connection_properties.erase("CLIENT_NO_SCHEMA");

    /* 13) MYSQL_OPT_CONNECT_TIMEOUT */
    connection_properties.erase("OPT_CONNECT_TIMEOUT");
    {
      logMsg("... testing OPT_CONNECT_TIMEOUT");
      sql::ConnectPropertyVal tmp;
      /* C-API does not care about the actual value */
      tmp.lval=(long long) - 1;
      connection_properties[std::string("OPT_CONNECT_TIMEOUT")]=tmp;
      try
      {
        created_objects.clear();
        con.reset(driver->connect(connection_properties));
      }
      catch (sql::SQLException &e)
      {
        fail(e.what(), __FILE__, __LINE__);
      }
    }
    connection_properties.erase("OPT_CONNECT_TIMEOUT");

    /* 14) MYSQL_OPT_READ_TIMEOUT */
    connection_properties.erase("OPT_READ_TIMEOUT");
    {
      logMsg("... testing OPT_READ_TIMEOUT");
      sql::ConnectPropertyVal tmp;
      /* C-API does not care about the actual value */
      tmp.lval=(long long) - 1;
      connection_properties[std::string("OPT_READ_TIMEOUT")]=tmp;
      try
      {
        created_objects.clear();
        con.reset(driver->connect(connection_properties));
      }
      catch (sql::SQLException &e)
      {
        fail(e.what(), __FILE__, __LINE__);
      }
    }
    connection_properties.erase("OPT_READ_TIMEOUT");

    /* 15) MYSQL_OPT_WRITE_TIMEOUT */
    connection_properties.erase("OPT_WRITE_TIMEOUT");
    {
      logMsg("... testing OPT_WRITE_TIMEOUT");
      sql::ConnectPropertyVal tmp;
      /* C-API does not care about the actual value */
      tmp.lval=(long long) - 1;
      connection_properties[std::string("OPT_WRITE_TIMEOUT")]=tmp;
      try
      {
        created_objects.clear();
        con.reset(driver->connect(connection_properties));
      }
      catch (sql::SQLException &e)
      {
        fail(e.what(), __FILE__, __LINE__);
      }
    }
    connection_properties.erase("OPT_WRITE_TIMEOUT");

    /* 16) MYSQL_OPT_RECONNECT */
    connection_properties.erase("OPT_RECONNECT");
    {
      logMsg("... testing OPT_RECONNECT");
      sql::ConnectPropertyVal tmp;
      /* C-API does not care about the actual value */
      tmp.lval=(long long) - 1;
      connection_properties[std::string("OPT_RECONNECT")]=tmp;
      try
      {
        created_objects.clear();
        con.reset(driver->connect(connection_properties));
      }
      catch (sql::SQLException &e)
      {
        fail(e.what(), __FILE__, __LINE__);
      }
    }
    connection_properties.erase("OPT_RECONNECT");

    /* 17) MYSQL_OPT_SET_CHARSET_NAME */
    connection_properties.erase("OPT_SET_CHARSET_NAME");
    {
      logMsg("... testing OPT_SET_CHARSET_NAME");
      std::string charset("utf8");
      sql::ConnectPropertyVal tmp;
      /* C-API does not care about the actual value */
      tmp.str.val=charset.c_str();
      tmp.str.len=charset.length();
      connection_properties[std::string("OPT_SET_CHARSET_NAME")]=tmp;
      try
      {
        created_objects.clear();
        con.reset(driver->connect(connection_properties));
      }
      catch (sql::SQLException &e)
      {
        fail(e.what(), __FILE__, __LINE__);
      }
    }
    connection_properties.erase("OPT_SET_CHARSET_NAME");

    /* 18) MYSQL_REPORT_DATA_TRUNCATION */
    connection_properties.erase("REPORT_DATA_TRUNCATION");
    {
      logMsg("... testing REPORT_DATA_TRUNCATION");
      std::string charset("1");
      sql::ConnectPropertyVal tmp;
      /* C-API does not care about the actual value */
      tmp.str.val=charset.c_str();
      tmp.str.len=charset.length();
      connection_properties[std::string("REPORT_DATA_TRUNCATION")]=tmp;
      try
      {
        created_objects.clear();
        con.reset(driver->connect(connection_properties));
      }
      catch (sql::SQLException &e)
      {
        fail(e.what(), __FILE__, __LINE__);
      }
    }
    connection_properties.erase("REPORT_DATA_TRUNCATION");


    /* 19) metadataUseInfoSchema */
    connection_properties.erase("metadataUseInfoSchema");
    {
      logMsg("... testing metadataUseInfoSchema");
      sql::ConnectPropertyVal tmp;
      tmp.bval=true;
      connection_properties[std::string("metadataUseInfoSchema")]=tmp;
      try
      {
        created_objects.clear();
        con.reset(driver->connect(connection_properties));
      }
      catch (sql::SQLException &e)
      {
        fail(e.what(), __FILE__, __LINE__);
      }

      connection_properties.erase("metadataUseInfoSchema");
      tmp.bval=false;
      connection_properties[std::string("metadataUseInfoSchema")]=tmp;
      try
      {
        created_objects.clear();
        con.reset(driver->connect(connection_properties));
      }
      catch (sql::SQLException &e)
      {
        fail(e.what(), __FILE__, __LINE__);
      }
    }
    connection_properties.erase("metadataUseInfoSchema");



  }
  catch (sql::SQLException &e)
  {
    logErr(e.what());
    logErr("SQLState: " + e.getSQLState());
    fail(e.what(), __FILE__, __LINE__);
  }

}

void connection::setTransactionIsolation()
{
  logMsg("connection::setTransactionIsolation() - MySQL_Connection::setTransactionIsolation()");
  stmt.reset(con->createStatement());
  try
  {
    con->setTransactionIsolation(sql::TRANSACTION_READ_COMMITTED);
    res.reset(stmt->executeQuery("SHOW VARIABLES LIKE 'tx_isolation'"));
    checkResultSetScrolling(res);
    res->next();
    ASSERT_EQUALS("READ-COMMITTED", res->getString("Value"));
    
    con->setTransactionIsolation(sql::TRANSACTION_READ_UNCOMMITTED);
    res.reset(stmt->executeQuery("SHOW VARIABLES LIKE 'tx_isolation'"));
    res->next();
    ASSERT_EQUALS("READ-UNCOMMITTED", res->getString("Value"));

    con->setTransactionIsolation(sql::TRANSACTION_REPEATABLE_READ);
    res.reset(stmt->executeQuery("SHOW VARIABLES LIKE 'tx_isolation'"));
    res->next();
    ASSERT_EQUALS("REPEATABLE-READ", res->getString("Value"));

    con->setTransactionIsolation(sql::TRANSACTION_SERIALIZABLE);
    res.reset(stmt->executeQuery("SHOW VARIABLES LIKE 'tx_isolation'"));
    res->next();
    ASSERT_EQUALS("SERIALIZABLE", res->getString("Value"));

  }
  catch (sql::SQLException &e)
  {
    logErr(e.what());
    logErr("SQLState: " + e.getSQLState());
    fail(e.what(), __FILE__, __LINE__);
  }
}


/*
 TODO - do we want to add this to sql::Connection?
void connection::setSessionVariable()
{
  logMsg("connection::setSessionVariable() - MySQL_Connection::setSessionVariable()");
  try
  {
    con->setSessionVariable("autocommit", "OFF");
    con->setSessionVariable("sql_mode", "ANSI");
  }
  catch (sql::SQLException &e)
  {
    logErr(e.what());
    logErr("SQLState: " + e.getSQLState());
    fail(e.what(), __FILE__, __LINE__);
  }

  con->close();
  try
  {
    con->setSessionVariable("autocommit", "OFF");
    FAIL("Closed connection not detected");
  }
  catch (sql::SQLException &)
  {
  }

}
 */

} /* namespace connection */
} /* namespace testsuite */
