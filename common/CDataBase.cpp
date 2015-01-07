#include "../common/CDataBase.h"

CDataBase::RowSet::RowSet(MYSQL *mySql, const char *query, int *res)
{
	m_sql = mySql;
	m_res = NULL;
	m_bValid = false;

	int result = mysql_query(m_sql, query);
	if ( !result )
	{
		m_res = mysql_store_result(m_sql);
		if ( !m_res )
		{
			const char *myErr = mysql_error(m_sql);
			if ( *myErr )
				g_Log.EventError("SQL \"%s\" returned an error \"%s\"" DEBUG_CR, query, myErr);
		}
		m_bValid = true;
	}
	else
	{
		const char *myErr = mysql_error(m_sql);
		if ( *myErr )
			g_Log.EventError("SQL \"%s\" returned an error \"%s\"" DEBUG_CR, query, myErr);
	}
	if ( res ) *res = result;
}

CDataBase::RowSet::~RowSet()
{
	if ( m_res )
		mysql_free_result(m_res);
}

void CDataBase::RowSet::getString(char *buf, UINT column, UINT row)
{
	MYSQL_ROW	dbrow;
	int			i = 0;

	//	since 0-row is a row of headers, start with index of 1
	while ( dbrow = mysql_fetch_row(m_res) )
	{
		if ( i++ == (row+1) )
			break;
	}

	if ( !dbrow || !dbrow[column] )	//	NULL value or non-existant field
		*buf = 0;
	else
		strcpy(buf, dbrow[column]);
}

UINT CDataBase::RowSet::getUINT(UINT column, UINT row)
{
	char	*buf = Str_GetTemp();
	getString(buf, column, row);
	return atol(buf);
}

double CDataBase::RowSet::getDouble(UINT column, UINT row)
{
	char	*buf = Str_GetTemp();
	getString(buf, column, row);
	return atof(buf);
}

bool CDataBase::RowSet::isValid()
{
	return m_bValid;
}

UINT CDataBase::RowSet::getRows()
{
	return mysql_num_rows(m_res);
}

//
//	///////////////////////////////////////////////////////////////////////////
//

CDataBase::CDataBase()
{
	_bConnected = false;
}

CDataBase::~CDataBase()
{
	if ( _bConnected )
		Close();
}

bool CDataBase::Connect(const char *user, const char *password, const char *base, const char *host)
{
	_myData = mysql_init(NULL);
	if ( !_myData )
		return false;
	if ( !mysql_real_connect(_myData, host, user, password, base, MYSQL_PORT, NULL, 0 ) )
	{
		mysql_close(_myData);
		return false;
	}
	return (_bConnected = true);
}

bool CDataBase::isConnected()
{
	return _bConnected;
}

void CDataBase::Close()
{
	mysql_close(_myData);
	_bConnected = false;
}

CDataBase::RowSet CDataBase::query(const char *query)
{
	if ( !_bConnected )
		throw CException(LOGL_CRIT, 0, "Cannot execute SQL query since the DB is not available!");

	int					result;
	CDataBase::RowSet	r(_myData, query, &result);

	if (( result == CR_SERVER_GONE_ERROR ) || ( result == CR_SERVER_LOST ))
		Close();
	return r;
}

CDataBase::RowSet __cdecl CDataBase::queryf(char *fmt, ...)
{
	char	*buf = Str_GetTemp();
	va_list	marker;

	va_start(marker, fmt);
	vsprintf(buf, fmt, marker);
	va_end(marker);

	return this->query(buf);
}

void CDataBase::exec(const char *query)
{
	if ( mysql_query(_myData, query) )
	{
		const char *myErr = mysql_error(_myData);
		if ( *myErr )
			g_Log.EventError("SQL \"%s\" returned an error \"%s\"" DEBUG_CR, query, myErr);
	}
}

void __cdecl CDataBase::execf(char *fmt, ...)
{
	char	*buf = Str_GetTemp();
	va_list	marker;

	va_start(marker, fmt);
	vsprintf(buf, fmt, marker);
	va_end(marker);

	this->exec(buf);
}

UINT CDataBase::getLastId()
{
	return mysql_insert_id(_myData);
}

void CDataBase::OnTick()
{
	EXC_TRY(("OnTick()"));

	if ( !g_Cfg.m_bMySql )	//	mySQL is not supported
		return;

	if ( _bConnected )	//	currently connected - just check that the link is alive
	{
		if ( mysql_ping(_myData) )
		{
			g_Log.EventError("MySQL server link has been lost. Trying to reattach to it" DEBUG_CR);
			Close();
		}
	}

	if ( !_bConnected )	//	the link with the server is lost -> try to connect
	{
		if ( !Connect(g_Cfg.m_sMySqlUser, g_Cfg.m_sMySqlPass, g_Cfg.m_sMySqlDB, g_Cfg.m_sMySqlHost) )
		{
			g_Log.EventError("MySQL reattach failed/timed out. SQL operations disabled." DEBUG_CR);
		}
	}

	EXC_CATCH("CDataBase");
}
