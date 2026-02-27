#include "mqtb370.h"

static int set_defaults(MQTB *mqtb);
static int process_configuration(MQTB *mqtb);
static int setLuaPath(MQTB *mqtb, const char* path);
static void dumpstack (MQTB *mqtb, const char *funcname);

int mqtb_config(MQTB *mqtb, int argc, char **argv)
{
    int         rc      = 8;
    CLIBCRT		*crt 	= __crtget();
	lua_State	*L		= NULL;
    const char  *config = NULL;
    int         i;
	char 		mem[9];
	char 		dataset[56];

	/* put our MQTB pointer in the CRT */
	crt->crtapp1 = mqtb;

	/* look for "CONFIG=" parm */
	for(i=1; i < argc; i++) {
		// wtof("%s: argv[%d]=\"%s\"", __func__, i, argv[i]);
		if (memcmp(argv[i], "CONFIG=", 7)==0) {
			config = &argv[i][7];
			while(*config=='=' || isspace(*config)) config++; 
			if (!*config) config = "CONFIG";
		}
	}

	if (config) {
		if (strchr(config, '.')) {
			/* config is likely a datset name */
			for(i=0; i < sizeof(dataset); i++) {
				dataset[i] = toupper(config[i]);
				if (config[i]==0) break;
			}
			dataset[sizeof(dataset)-1]=0;
		}
		else {
			/* build configuration dataset name */
			for(i=0; i < sizeof(mem); i++) {
				mem[i] = toupper(config[i]);
				if (config[i]==0) break;
			}
			mem[8] = 0;
			snprintf(dataset, sizeof(dataset),"DD:MQTTDLUA(%s)", mem);
		}
	}

	/* create new Lua state */
	mqtb->config = luaL_newstate();
	if (!mqtb->config) {
        wtof("MQTTD100E Unable to create Lua state for configuration");
        goto quit;
    }

	/* Open Lua libraries */
	luaL_openlibs(mqtb->config);

	/* Set the search path for require commands */
	setLuaPath(mqtb,"DD:MQTTDLUA(?);DD:LUALIB(?)");

	/* make WTO available */
	luaL_dostring(mqtb->config, "wto = os.wto");

	/* set default */
	set_defaults(mqtb);

	/* did the user provide a member or dataset to execute? */
	if (config) {
		/* yes, execute Lua script */
		rc = luaL_dofile(mqtb->config, dataset);
		if (rc) {
			/* Something went wrong, dump stack to console */
			dumpstack(mqtb,__func__);
		}
	}
	
	/* process the configuration */
	rc = process_configuration(mqtb);
    if (rc) {
        wtof("MQTTD101E Configuration Error");
        goto quit;
    }

    wtof("MQTTD102I MQTT Broker Configuration Complete");
    rc = 0;
    
quit:
    return rc;
}

static int set_defaults(MQTB *mqtb)
{
    lua_State   *L      = mqtb->config;

	// Create new table for "mqttd"
	lua_newtable(L);			// create new table

    lua_pushstring(L, "anonymous,login,list");
    lua_setfield(L,-2,"auth");      // table.auth="anonymous"
    
    lua_pushstring(L, "*");
    lua_setfield(L,-2,"auth_list"); // table.auth_list="*"

	lua_pushstring(L, "");
	lua_setfield(L,-2,"tz");	    // table.tz="0"

	lua_pushstring(L,"1883");
	lua_setfield(L,-2,"port");		// table.port="1883"
	
	lua_pushstring(L, "5");
	lua_setfield(L,-2,"listen_queue");// table.listen_queue="5"

	lua_pushstring(L, "10");
	lua_setfield(L,-2,"bind_tries");  // table.bind_tries="10"

	lua_pushstring(L, "10");
	lua_setfield(L,-2,"bind_sleep");  // table.bind_sleep="10"

	lua_pushstring(L, "3");	
	lua_setfield(L,-2,"mintask");	// table.mintask="3"
	
	lua_pushstring(L, "9");
	lua_setfield(L,-2,"maxtask");	// table.maxtask="9"
	
    lua_pushstring(L, "31536000");  
    lua_setfield(L,-2,"topic_expire");  // table.topic_expire="31536000" (1 Year)
    
    lua_pushstring(L, "");
    lua_setfield(L,-2,"retain_topics_dataset"); // table.retain_topics_dataset=""
    
	lua_setglobal(L, "mqttd");	// mqttd = table

    return 0;
}

static int process_configuration(MQTB *mqtb)
{
    int         rc      = 8;
    lua_State   *L      = mqtb->config;
    MQTBC       *mqtbc  = NULL;
	int			i;
	const char  *p;
    char        *tmp;
    char        *s;

    mqtbc_free(&mqtb->mqtbc);
    mqtbc = mqtb->mqtbc = mqtbc_new(mqtb);
    if (!mqtbc) {
        wtof("MQTTD999E Out of Memory");
        goto quit;
    }

	lua_getglobal(L, "mqttd");	// push mqttd table on top of stack

	lua_getfield(L,-1,"auth_list");
	p = lua_tostring(L, -1);
    if (p && *p) {
        tmp = strdup(p);
        if (tmp) {
            for(p=strtok(tmp, " ,;") ; p && *p ; p=strtok(NULL, " ,;")) {
                s = strdup(p);
                if (s) {
                    util_str_upper(s);
                    array_add(&mqtbc->auth_list, s);
                }
            }
            free(tmp);
        }
    }
	lua_pop(L,1);

	lua_getfield(L,-1,"auth");
	p = lua_tostring(L, -1);
    if (p && *p) {
        tmp = strdup(p);
        // wtof("%s: auth=\"%s\"", __func__, tmp);
        if (tmp) {
            for(p=strtok(tmp, " ,;") ; p && *p ; p=strtok(NULL, " ,;")) {
                s = strdup(p);
                // wtof("%s: s=\"%s\"", __func__, s);
                if (s) {
                    util_str_upper(s);
                    i = strlen(s);
                    // wtof("%s: s=\"%s\" i=%d", __func__, s, i);
                    if (memcmp(s, "ANONYMOUS", i)==0) {
                        // wtof("%s: s=\"%s\" OKAY", __func__, s);
                        mqtbc->auth |= MQTBC_AUTH_ANON;
                    }
                    else if (memcmp(s, "LOGIN", i)==0) {
                        // wtof("%s: s=\"%s\" OKAY", __func__, s);
                        mqtbc->auth |= MQTBC_AUTH_LOGIN;
                    }
                    else if (memcmp(s, "LIST", i)==0) {
                        // wtof("%s: s=\"%s\" OKAY", __func__, s);
                        mqtbc->auth |= MQTBC_AUTH_LIST;
                    }
                    free(s);
                }
            }
            free(tmp);
        }
    }
	lua_pop(L,1);
    
	lua_getfield(L,-1,"tz");
	p = lua_tostring(L, -1);
    if (p && *p) mqtbc->tz = strdup(p);
	lua_pop(L,1);

	lua_getfield(L,-1,"port");
	i = (int) lua_tointeger(L, -1);
	if (i>0) {
		if (i > 0xFFFF) i = 0xFFFF;
		mqtbc->port = (unsigned short)i;
	}
	else mqtbc->port = 1883;	// default
	lua_pop(L,1);

	lua_getfield(L,-1,"listen_queue");
	i = (int) lua_tointeger(L, -1);
	if (i>0) {
		if (i > 255) i = 255;
		mqtbc->listenq = (unsigned char)i;
	}
	else mqtbc->listenq = 5;	// default
	lua_pop(L,1);

	lua_getfield(L,-1,"bind_tries");
	i = (int) lua_tointeger(L, -1);
	if (i>0) {
		if (i > 255) i = 255;
		mqtbc->bind_tries = (unsigned char)i;
	}
	else mqtbc->bind_tries = 5;	// default
	lua_pop(L,1);

	lua_getfield(L,-1,"bind_sleep");
	i = (int) lua_tointeger(L, -1);
	if (i>0) {
		if (i > 255) i = 255;
		mqtbc->bind_sleep = (unsigned char)i;
	}
	else mqtbc->bind_sleep = 5;	// default
	lua_pop(L,1);

	lua_getfield(L,-1,"maxtask");
	i = (int) lua_tointeger(L, -1);
	if (i>0) {
		if (i > 255) i = 255;
		mqtbc->maxtask = (unsigned char)i;
	}
	else mqtbc->maxtask = 9;	// default
	lua_pop(L,1);

	lua_getfield(L,-1,"mintask");
	i = (int) lua_tointeger(L, -1);
	if (i>0) {
		if (i > 255) i = 255;
		mqtbc->mintask = (unsigned char)i;
	}
	else mqtbc->mintask = 3;	// default
	lua_pop(L,1);

    /* make sure that mintask <= maxtask */
	if (mqtbc->mintask > mqtbc->maxtask) {
		mqtbc->maxtask = mqtbc->mintask;
	}
	if (mqtbc->maxtask < mqtbc->mintask) {
		mqtbc->mintask = mqtbc->maxtask;
	}

	lua_getfield(L,-1,"topic_expire");
	i = (int) lua_tointeger(L, -1);
    if (i < 0) {
        /* set default 60 minutes seconds */
        mqtbc->topic_expire = (24 * 60 * 60 * 365);  /* 1 Year */
    }
    else {
		mqtbc->topic_expire = (unsigned)i;
	}
	lua_pop(L,1);

	lua_getfield(L,-1,"retain_topics_dataset");
	p = lua_tostring(L, -1);
    if (p && *p) {
        mqtbc->retain_dataset = strdup(p);
        util_str_upper(mqtbc->retain_dataset);
    }
	lua_pop(L,1);

    rc = 0;

quit:
	lua_pop(L,1);	// pop mqtb table off of stack

    return rc;
}

static int 
setLuaPath(MQTB *mqtb, const char* path)
{
    lua_State   *L      = mqtb->config;
	
	lua_getglobal( L, "package" );

	// wtof("%s: new path=\"%s\"", __func__, path);

    lua_pushstring( L, path ); // push the new one
    lua_setfield( L, -2, "path" ); // set the field "path" in table at -2 with value at top of stack
    lua_remove( L, -1 ); // get rid of package table from top of stack

    return 0; // all done!
}

static void 
dumpstack (MQTB *mqtb, const char *funcname) 
{
    lua_State   *L      = mqtb->config;
	int 		top		= lua_gettop(L);
	int			i;
	int			j;
	char		buf[256];
	
	wtof("%s Stack Dump (%d)", funcname, top);
	for (i=1, j=-top; i <= top; i++, j++) {
		const char *typename = luaL_typename(L,i);
		sprintf(buf, "%3d (%d) %.12s", i, j, luaL_typename(L,i));
		switch (lua_type(L, i)) {
			case LUA_TNUMBER:
				wtof("%s %g", buf, lua_tonumber(L,i));
				break;
			case LUA_TSTRING:
				wtof("%s %s", buf, lua_tostring(L,i));
				break;
			case LUA_TBOOLEAN:
				wtof("%s %s", buf, (lua_toboolean(L, i) ? "true" : "false"));
				break;
			case LUA_TNIL:
				wtof("%s %s", buf, "nil");
				break;
			default:
				wtof("%s %p", buf ,lua_topointer(L,i));
				break;
		}
	}
}
