#pragma once

#define veccpy(a,b) b[0] = a[0]; b[1] = a[1]; b[2] = a[2]; 
#define vecdist(v0,v1) sqrt((v1[1] - v0[1]) * (v1[1] - v0[1]) + (v1[0] - v0[0]) * (v1[0] - v0[0]) + (v1[2] - v0[2])* (v1[2] - v0[2]))
#define veczero(a) a[0] = 0; a[1] = 0; a[2] = 0;

const char* random_primary[4] = {
	"iw6_usr_mp_barrelbored_fmj_usrscope",
	"iw6_l115a3_mp_barrelbored_fmj_l115a3scope",
	"iw6_gm6_mp_barrelbored_fmj_gm6scope", // Lynx
	"iw6_vks_mp_barrelbored_fmj_vksscope"
};

const char* random_secondary[8] = {
	"iw6_magnum_mp",
	"iw6_mk14_mp",
	"iw6_p226_mp",
	"iw6_sc2010_mp",
	"iw6_pdw_mp",
	"iw6_honeybadger_mp",
	"iw6_k7_mp",
	"iw6_arx160_mp"
};

namespace trickshot {
	namespace {
		void randomize_class();
	}
}