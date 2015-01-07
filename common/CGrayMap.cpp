//
// CGrayMap.cpp
//

#include "../graysvr/graysvr.h"

//////////////////////////////////////////////////////////////////
// -CGrayMapBlockState

#ifndef CAN_I_BLOCK
#define CAN_I_BLOCK		UFLAG1_BLOCK
#define CAN_I_PLATFORM	UFLAG2_PLATFORM
#define CAN_I_CLIMB		UFLAG2_CLIMBABLE
#define CAN_I_DOOR		UFLAG4_DOOR
#endif

CGrayMapBlockState::CGrayMapBlockState( DWORD dwBlockFlags, signed char z, int iHeight ) :
	m_dwBlockFlags(dwBlockFlags),	m_z(z), m_iHeight(iHeight), m_zClimb(0)
{
	// m_z = PLAYER_HEIGHT
	m_Top.m_dwBlockFlags = 0;
	m_Top.m_wTile = 0;
	m_Top.m_z = UO_SIZE_Z;	// the z of the item that would be over our head.

	m_Bottom.m_dwBlockFlags = CAN_I_BLOCK; // The bottom item has these blocking flags.
	m_Bottom.m_wTile = 0;
	m_Bottom.m_z = UO_SIZE_MIN_Z;	// the z we would stand on,

	m_Lowest.m_dwBlockFlags = CAN_I_BLOCK; 
	m_Lowest.m_wTile = 0;
	m_Lowest.m_z = UO_SIZE_Z;

	m_zClimbHeight = 0;
}

CGrayMapBlockState::CGrayMapBlockState( DWORD dwBlockFlags, signed char z, int iHeight, signed char zClimb ) :
	m_dwBlockFlags(dwBlockFlags),	m_z(z), m_iHeight(iHeight), m_zClimb(zClimb)
{
	m_Top.m_dwBlockFlags = 0;
	m_Top.m_wTile = 0;
	m_Top.m_z = UO_SIZE_Z;	// the z of the item that would be over our head.

	m_Bottom.m_dwBlockFlags = CAN_I_BLOCK; // The bottom item has these blocking flags.
	m_Bottom.m_wTile = 0;
	m_Bottom.m_z = UO_SIZE_MIN_Z;	// the z we would stand on,

	m_Lowest.m_dwBlockFlags = CAN_I_BLOCK; 
	m_Lowest.m_wTile = 0;
	m_Lowest.m_z = UO_SIZE_Z;

	m_zClimbHeight = 0;
}

LPCTSTR CGrayMapBlockState::GetTileName( WORD wID )	// static
{
	if ( wID == 0 )
	{
		return( "<null>" );
	}
	TCHAR * pStr = Str_GetTemp();
	if ( wID < TERRAIN_QTY )
	{
		CGrayTerrainInfo land( wID );
		strcpy( pStr, land.m_name );
	}
	else
	{
		wID -= TERRAIN_QTY;
		CGrayItemInfo item( (ITEMID_TYPE) wID );
		strcpy( pStr, item.m_name );
	}
	return( pStr );
}

bool CGrayMapBlockState::CheckTile( DWORD wItemBlockFlags, signed char zBottom, signed char zHeight, WORD wID )
{
	// RETURN:
	//  true = continue processing

	signed char zTop = zBottom;
	if ( wItemBlockFlags & CAN_I_CLIMB )
		zTop += ( zHeight / 2 );	// standing position is half way up climbable items.
	else
		zTop += zHeight;

	if ( zTop < m_Bottom.m_z )	// below something i can already step on.
		return true;

	if ( ! wItemBlockFlags )	// no effect.
		return( true );

	// If this item does not block me at all then i guess it just does not count.
	if ( ! ( wItemBlockFlags &~ m_dwBlockFlags ))
	{	// this does not block me.
		if ( ! ( wItemBlockFlags & CAN_I_CLIMB ))
		{
			if ( wItemBlockFlags & CAN_I_PLATFORM )	// i can always walk on the platform.
			{
				zBottom = zTop;	// or i could walk under it.
			}
			else
			{
				zTop = zBottom;
			}
			// zHeight = 0;	// no effective height.
		}
	}

	if ( zTop < m_Lowest.m_z )
	{
		m_Lowest.m_dwBlockFlags = wItemBlockFlags;
		m_Lowest.m_wTile = wID;
		m_Lowest.m_z = zTop;
	}

	// if i can't fit under this anyhow. it is something below me. (potentially)
	if ( zBottom < ( m_z + m_iHeight/2 ))
	{
		// This is the new item ( that would be ) under me.
		// NOTE: Platform items should take precendence over non-platforms.
		if ( zTop >= m_Bottom.m_z )
		{
			if ( zTop == m_Bottom.m_z )
			{
				if ( m_Bottom.m_dwBlockFlags & CAN_I_PLATFORM )
					return( true );
			}
			m_Bottom.m_dwBlockFlags = wItemBlockFlags;
			m_Bottom.m_wTile = wID;
			m_Bottom.m_z = zTop;
		}
	}
	else
	{
		// I could potentially fit under this. ( it would be above me )
		if ( zBottom <= m_Top.m_z )
		{
			m_Top.m_dwBlockFlags = wItemBlockFlags;
			m_Top.m_wTile = wID;
			m_Top.m_z = zBottom;
		}
	}

	return true;
}

bool CGrayMapBlockState::CheckTile_Item( DWORD wItemBlockFlags, signed char zBottom, signed char zHeight, WORD wID )
{
	// RETURN:
	//  true = continue processing

	if ( ! wItemBlockFlags )	// no effect.
		return( true );

	signed char zTop = zBottom;
	if ( wItemBlockFlags & CAN_I_CLIMB )
		zTop += ( zHeight / 2 );
	else
		zTop += zHeight;
	if ( zTop < m_Bottom.m_z )	// below something i can already step on.
		return true;

	if ( zTop < m_Lowest.m_z )
	{
		m_Lowest.m_dwBlockFlags = wItemBlockFlags;
		m_Lowest.m_wTile = wID;
		m_Lowest.m_z = zTop;
	}

	if ( ! ( wItemBlockFlags &~ m_dwBlockFlags ))
	{	// this does not block me.
		if ( ! ( wItemBlockFlags & ( CAN_I_CLIMB | CAN_I_PLATFORM ) ) )
		{
			return true;
		}
	}

	if ( ( zBottom <= m_zClimb ) )
	{
		if ( zTop >= m_Bottom.m_z )
		{
			if ( zTop == m_Bottom.m_z )
			{
				if ( ! ( wItemBlockFlags & CAN_I_CLIMB ) ) // climbable items have the highest priority
					if ( m_Bottom.m_dwBlockFlags & CAN_I_PLATFORM ) //than items with CAN_I_PLATFORM
						return ( true );
			}
			m_Bottom.m_dwBlockFlags = wItemBlockFlags;
			m_Bottom.m_wTile = wID;
			m_Bottom.m_z = zTop;

			if ( wItemBlockFlags & CAN_I_CLIMB ) // return climb height
				m_zClimbHeight = ( zHeight / 2 );
			else
				m_zClimbHeight = 0;
		}
	}
	else
	{
		// I could potentially fit under this. ( it would be above me )
		if ( zBottom < m_Top.m_z )
		{
			m_Top.m_dwBlockFlags = wItemBlockFlags;
			m_Top.m_wTile = wID;
			m_Top.m_z = zBottom;
		}
	}
	return true;

}

bool CGrayMapBlockState::CheckTile_Terrain( DWORD wItemBlockFlags, signed char z, WORD wID )
{
	// RETURN:
	//  true = continue processing

	if ( ! wItemBlockFlags )	// no effect.
		return( true );

	if ( z < m_Bottom.m_z )	// below something i can already step on.
		return true;

	if ( z < m_Lowest.m_z )
	{
		m_Lowest.m_dwBlockFlags = wItemBlockFlags;
		m_Lowest.m_wTile = wID;
		m_Lowest.m_z = z;
	}

	if	(  z <= m_iHeight )
	{
		if ( z >= m_Bottom.m_z )
		{
			if ( z == m_Bottom.m_z )
			{
				if ( m_Bottom.m_dwBlockFlags & CAN_I_CLIMB ) // climbable items have the highest priority
					return ( true );
				if ( ( m_Bottom.m_dwBlockFlags & CAN_I_PLATFORM ) && (!( wItemBlockFlags & CAN_I_PLATFORM )) )
					return ( true );
			} else if ( z > m_z + PLAYER_HEIGHT/2 ) 
				if ( (m_Bottom.m_dwBlockFlags & (CAN_I_PLATFORM|CAN_I_CLIMB)) && (z >= m_Bottom.m_z + PLAYER_HEIGHT/2) ) // we can walk under it
					goto settop;

			m_Bottom.m_dwBlockFlags = wItemBlockFlags;
			m_Bottom.m_wTile = wID;
			m_Bottom.m_z = z;
			m_zClimbHeight = 0;
		}
	}
	else
	{
		// I could potentially fit under this. ( it would be above me )
settop:
		if ( z < m_Top.m_z )
		{
			m_Top.m_dwBlockFlags = wItemBlockFlags;
			m_Top.m_wTile = wID;
			m_Top.m_z = z;
		}
	}
	return true;
}


//////////////////////////////////////////////////////////////////
// -CGrayStaticsBlock

void CGrayStaticsBlock::LoadStatics( DWORD ulBlockIndex, int map )
{
	// long ulBlockIndex = (bx*(UO_SIZE_Y/UO_BLOCK_SIZE) + by);
	// NOTE: What is index.m_wVal3 and index.m_wVal4 in VERFILE_STAIDX ?
	ASSERT( m_iStatics <= 0 );

	CUOIndexRec index;
	if ( g_Install.ReadMulIndex(g_Install.m_Staidx[g_MapList.m_mapnum[map]], ulBlockIndex, index) )
	{
		m_iStatics = index.GetBlockLength()/sizeof(CUOStaticItemRec);
		ASSERT(m_iStatics);
		m_pStatics = new CUOStaticItemRec[m_iStatics];
		ASSERT(m_pStatics);
		if ( ! g_Install.ReadMulData(g_Install.m_Statics[g_MapList.m_mapnum[map]], index, m_pStatics) )
		{
			throw CGrayError(LOGL_CRIT, CGFile::GetLastError(), "CGrayMapBlock: Read fStatics0");
		}
	}
}

//////////////////////////////////////////////////////////////////
// -CGrayMapBlock

int CGrayMapBlock::sm_iCount = 0;

void CGrayMapBlock::Load( int bx, int by )
{
	// Read in all the statics data for this block.
	m_CacheTime.InitCacheTime();		// This is invalid !

	ASSERT( bx < (g_MapList.GetX(m_map)/UO_BLOCK_SIZE) );
	ASSERT( by < (g_MapList.GetX(m_map)/UO_BLOCK_SIZE) );

	if (( m_map < 0 ) || ( m_map >= 255 ))
	{
		g_Log.EventError("Unsupported map #%d specifyed. Auto-fixing that to 0." DEBUG_CR, m_map);
		m_map = 0;
	}

	long ulBlockIndex = (bx*(g_MapList.GetY(m_map)/UO_BLOCK_SIZE) + by);

	if ( !g_MapList.m_maps[m_map] )
	{
		memset( &m_Terrain, 0, sizeof( m_Terrain ));
		throw CGrayError(LOGL_CRIT, 0, "CGrayMapBlock: Map is not supported since MUL files for it not available.");
	}

	CGFile * pFile;
	CUOIndexRec index;
	index.SetupIndex( ulBlockIndex * sizeof(CUOMapBlock), sizeof(CUOMapBlock));
	pFile = &(g_Install.m_Maps[g_MapList.m_mapnum[m_map]]);

	if ( pFile->Seek( index.GetFileOffset(), SEEK_SET ) != index.GetFileOffset() )
	{
		memset( &m_Terrain, 0, sizeof( m_Terrain ));
		throw CGrayError(LOGL_CRIT, CGFile::GetLastError(), "CGrayMapBlock: Seek Ver");
	}
	if ( pFile->Read( &m_Terrain, sizeof(CUOMapBlock)) <= 0 )
	{
		memset( &m_Terrain, 0, sizeof( m_Terrain ));
		throw CGrayError(LOGL_CRIT, CGFile::GetLastError(), "CGrayMapBlock: Read");
	}

	m_Statics.LoadStatics(ulBlockIndex, m_map);
	m_CacheTime.HitCacheTime();		// validate.
}

/*

  Vjaka -	here lies mapdiff support sample code from wolfpack
			for investigation purposes

QMap<unsigned int, unsigned int> mappatches;
QMap<unsigned int, stIndexRecord> staticpatches;

struct mapblock
{
	unsigned int header;
	map_st cells[64];
};

void MapsPrivate::loadDiffs( const QString& basePath, unsigned int id )
{
	QDir baseFolder( basePath );
	QStringList files = baseFolder.entryList();
	QString mapDiffListName = QString( "mapdifl%1.mul" ).arg( id );
	QString mapDiffFileName = QString( "mapdif%1.mul" ).arg( id );
	QString statDiffFileName = QString( "stadif%1.mul" ).arg( id );
	QString statDiffListName = QString( "stadifl%1.mul" ).arg( id );
	QString statDiffIndexName = QString( "stadifi%1.mul" ).arg( id );
	for ( QStringList::const_iterator it = files.begin(); it != files.end(); ++it )
	{
		if ( ( *it ).lower() == mapDiffListName )
			mapDiffListName = *it;
		else if ( ( *it ).lower() == mapDiffFileName )
			mapDiffFileName = *it;
		else if ( ( *it ).lower() == statDiffFileName )
			statDiffFileName = *it;
		else if ( ( *it ).lower() == statDiffListName )
			statDiffListName = *it;
		else if ( ( *it ).lower() == statDiffIndexName )
			statDiffIndexName = *it;
	}

	QFile mapdiflist( basePath + mapDiffListName );
	mapdifdata.setName( basePath + mapDiffFileName );
	
	// Try to read a list of ids
	if ( mapdifdata.open( IO_ReadOnly ) && mapdiflist.open( IO_ReadOnly ) )
	{
		QDataStream listinput( &mapdiflist );
		listinput.setByteOrder( QDataStream::LittleEndian );
		unsigned int offset = 0;
		while ( !listinput.atEnd() )
		{
			unsigned int id;
			listinput >> id;
			mappatches.insert( id, offset );
			offset += sizeof( mapblock );
		}
		mapdiflist.close();
	}

	stadifdata.setName( basePath + statDiffFileName );
	stadifdata.open( IO_ReadOnly );

	QFile stadiflist( basePath + statDiffListName );
	QFile stadifindex( basePath + statDiffIndexName );

	if ( stadifindex.open( IO_ReadOnly ) && stadiflist.open( IO_ReadOnly ) )
	{
		QDataStream listinput( &stadiflist );
		QDataStream indexinput( &stadifindex );
		listinput.setByteOrder( QDataStream::LittleEndian );
		indexinput.setByteOrder( QDataStream::LittleEndian );

		stIndexRecord record;
		while ( !listinput.atEnd() )
		{
			unsigned int id;
			listinput >> id;

			indexinput >> record.offset;
			indexinput >> record.blocklength;
			indexinput >> record.extra;

			if ( !staticpatches.contains( id ) )
			{
				staticpatches.insert( id, record );
			}
		}
	}

	if ( stadiflist.isOpen() )
	{
		stadiflist.close();
	}

	if ( stadifindex.isOpen() )
	{
		stadifindex.close();
	}
}

map_st MapsPrivate::seekMap( ushort x, ushort y )
{
	// The blockid our cell is in
	unsigned int blockid = x / 8 * height + y / 8;

	// See if the block has been cached
	mapblock* result = mapCache.find( blockid );
	bool borrowed = true;

	if ( !result )
	{
		result = new mapblock;

		// See if the block has been patched
		if ( mappatches.contains( blockid ) )
		{
			unsigned int offset = mappatches[blockid];
			mapdifdata.at( offset );
			mapdifdata.readBlock( ( char * ) result, sizeof( mapblock ) );
		}
		else
		{
			mapfile.at( blockid * sizeof( mapblock ) );
			mapfile.readBlock( ( char * ) result, sizeof( mapblock ) );
		}

		borrowed = mapCache.insert( blockid, result );
	}

	// Convert to in-block values.
	y %= 8;
	x %= 8;
	map_st cell = result->cells[y * 8 + x];

	if ( !borrowed )
		delete result;

	return cell;
}
*/