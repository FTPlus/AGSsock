/******************************************************************
 * Socket data interface -- See header file for more information. *
 ******************************************************************/

#include "API.h"
#include "SockData.h"

namespace AGSSock {

//------------------------------------------------------------------------------

int AGSSockData::Dispose(const char *data, bool force)
{
	delete (SockData *) data;
	return 1;
}

//------------------------------------------------------------------------------

int AGSSockData::Serialize(const char *data, char *buffer, int size)
{
	return ((SockData *) data)->data.copy(buffer, size);
}

//------------------------------------------------------------------------------

void AGSSockData::Unserialize(int key, const char *buffer, int size)
{
	SockData *data = new SockData(std::string(buffer, size));
	AGS_RESTORE(SockData, data, key);
}

//==============================================================================

SockData *SockData_Create(ags_t size, ags_t byte)
{
	SockData *data = new SockData(size, byte);
	AGS_OBJECT(SockData, data);
	return data;
}

//------------------------------------------------------------------------------

SockData *SockData_CreateEmpty()
{
	SockData *data = new SockData();
	AGS_OBJECT(SockData, data);
	return data;
}

//------------------------------------------------------------------------------

SockData *SockData_CreateFromString(const char *str)
{
	SockData *data = new SockData(str);
	AGS_OBJECT(SockData, data);
	return data;
}

//------------------------------------------------------------------------------

ags_t SockData_get_Size(SockData *sd)
{
	return sd->data.size();
}

//------------------------------------------------------------------------------

void SockData_set_Size(SockData *sd, ags_t size)
{
	sd->data.resize((size_t) size);
}

//------------------------------------------------------------------------------

ags_t SockData_geti_Chars(SockData *sd, ags_t index)
{
	return sd->data[index];
}

//------------------------------------------------------------------------------

void SockData_seti_Chars(SockData *sd, ags_t index, ags_t byte)
{
	sd->data[index] = (char) byte;
}

//------------------------------------------------------------------------------

const char *SockData_AsString(SockData *sd)
{
	return AGS_STRING(sd->data.c_str());
}

//------------------------------------------------------------------------------

void SockData_Clear(SockData *sd)
{
	sd->data.clear();
}

//------------------------------------------------------------------------------

} /* namespace AGSSock */

//..............................................................................
