/*
 * 
 */

#include "vfd.h"
#include "status.h"

#include <getopt.h>

#include "SysTime.h"

const char *cPluginVFD::VERSION        = "0.0.1";
const char *cPluginVFD::DESCRIPTION    = "Kathrein UFS910 VFD Plugin";

cPluginVFD::cPluginVFD(void) {
	m_Status = NULL;
	m_SysTime = NULL;
}

cPluginVFD::~cPluginVFD() {
	delete m_Status;
	delete m_SysTime;
}

bool cPluginVFD::Initialize(void) {
	return true;
}

bool cPluginVFD::Start(void) {
	m_Status = new cVFDStatus();
	m_SysTime = new cSysTime();
	return true;
}

void cPluginVFD::Housekeeping(void) {
}

cMenuSetupPage *cPluginVFD::SetupMenu(void) {
	return NULL;
}

bool cPluginVFD::SetupParse(const char *Name, const char *Value) {
	return false;
}

VDRPLUGINCREATOR(cPluginVFD);
