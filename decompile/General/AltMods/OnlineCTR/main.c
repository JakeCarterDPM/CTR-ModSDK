void octr_entryHook()
{
	// Until VehBirth_TeleportSelf is rewritten
	// AdvHub, replace default spawn with unused
	// start-line spawn, turn BEQ to JMP
	*(char*)0x80058072 = 0;
	
	// ======== Globals ============
	
	#if USE_K1 == 0
	struct OnlineCTR* octr = (struct OnlineCTR*)0x8000C000;
	#else
	octr = 0x8000C000;
	#endif
	
	// default for first LEV, before gameplay
	octr->DriverID = 0;
	octr->NumDrivers = 8;
	octr->IsBootedPS1 = 1;

	// FSM for menus
	octr->funcs[LAUNCH_ENTER_PID] = StatePS1_Launch_EnterPID;
	octr->funcs[LAUNCH_ENTER_IP] = StatePS1_Launch_EnterIP;
	octr->funcs[LAUNCH_CONNECT_FAILED] = StatePS1_Launch_ConnectFailed;
	octr->funcs[LAUNCH_FIRST_INIT] = StatePS1_Launch_FirstInit;
	octr->funcs[LOBBY_ASSIGN_ROLE] = StatePS1_Lobby_AssignRole;
	octr->funcs[LOBBY_HOST_TRACK_PICK] = StatePS1_Lobby_HostTrackPick;
	octr->funcs[LOBBY_GUEST_TRACK_WAIT] = StatePS1_Lobby_GuestTrackWait;
	octr->funcs[LOBBY_CHARACTER_PICK] = StatePS1_Lobby_CharacterPick;
	octr->funcs[LOBBY_WAIT_FOR_LOADING] = StatePS1_Lobby_WaitForLoading;
	octr->funcs[LOBBY_START_LOADING] = StatePS1_Lobby_StartLoading;
	octr->funcs[GAME_WAIT_FOR_RACE] = StatePS1_Game_WaitForRace;
	octr->funcs[GAME_START_RACE] = StatePS1_Game_StartRace;
}

void ThreadFunc()
{
	int i;
	
	#if USE_K1 == 0
	struct OnlineCTR* octr = (struct OnlineCTR*)0x8000C000;
	#endif
	
	for(i = 3; i >= 0; i--)
		octr->time[i+1] = octr->time[i];
	
	for(i = 3; i >= 0; i--)
		if(octr->time[i+1] != octr->time[i])
			break;
		
	// if client didn't update the game in 4 frames
	if(
		(i == -1) &&
		(octr->CurrState >= LAUNCH_FIRST_INIT)
	  )
	{
		// reset, including CurrState
		memset(octr, 0, sizeof(struct OnlineCTR));
		sdata->ptrActiveMenu = 0;
		octr_entryHook();
	}
	
	octr->funcs[octr->CurrState]();
}

// this runs after the end of MainInit_FinalizeInit,
// which is also after the end of camera initialization
void octr_initHook()
{
	#if USE_K1 == 0
	struct OnlineCTR* octr = (struct OnlineCTR*)0x8000C000;
	#endif
	
	// small stack pool, pause thread (those threads can't pause)
	PROC_BirthWithObject(0x310, ThreadFunc, 0, 0);
	
	sdata->lngStrings[0x17D] = "OnlineCTR";
}

// replace MainInit_Drivers
void OnlineInit_Drivers(struct GameTracker* gGT)
{	
	int i;
	int bitFlag;
	struct Driver* dr;

	#if USE_K1 == 0
	struct OnlineCTR* octr = (struct OnlineCTR*)0x8000C000;
	#endif

	for(i = 0; i < 8; i++)
	{
		gGT->drivers[i] = 0;
		sdata->kartSpawnOrderArray[i] = i;
	}
	
	// if first boot
	if(gGT->levelID == CREDITS_POLAR)
	{
		gGT->drivers[0] = VehBirth_Player(0);
		return;
	}
	
	if(DECOMP_LOAD_IsOpen_RacingOrBattle())
		DECOMP_RB_MinePool_Init();
	
	// 8 spawn positions
	bitFlag = 7;
	
	// override for battle maps
	if(
		(gGT->levelID > TURBO_TRACK) &&
		(gGT->levelID < GEM_STONE_VALLEY)
	  )
	{
		// 4 spawn positions
		bitFlag = 3;
	}
	
	// All clients must spawn drivers in the same order,
	// so that pointers can be sent over network
	
	for(i = 0; i < octr->DriverID; i++)
	{
		// init, save, fakeID, teleport, realID
		dr = DECOMP_VehBirth_Player(i+1);
		gGT->drivers[i+1] = dr;
		dr->driverID = i&bitFlag;
		VehBirth_TeleportSelf(dr,3,0);
		dr->driverID = i+1;
		
		#ifdef USE_60FPS
		// needed cause VehBirth_TeleportSelf
		// has not been rewritten yet for decomp
		dr->instSelf->animFrame = FPS_DOUBLE(10);
		#endif
	}
	
	// init, save, fakeID, teleport, realID
	dr = DECOMP_VehBirth_Player(0);
	gGT->drivers[0] = dr;
	dr->driverID = i&bitFlag;
	VehBirth_TeleportSelf(dr,3,0);
	dr->driverID = 0;
	
	#ifdef USE_60FPS
	// needed cause VehBirth_TeleportSelf
	// has not been rewritten yet for decomp
	dr->instSelf->animFrame = FPS_DOUBLE(10);
	#endif
	
	for(i = i+1; i < octr->NumDrivers; i++)
	{
		// init, save, fakeID, teleport, realID
		dr = DECOMP_VehBirth_Player(i);
		gGT->drivers[i] = dr;
		dr->driverID = i&bitFlag;
		VehBirth_TeleportSelf(dr,3,0);
		dr->driverID = i;
		
		#ifdef USE_60FPS
		// needed cause VehBirth_TeleportSelf
		// has not been rewritten yet for decomp
		dr->instSelf->animFrame = FPS_DOUBLE(10);
		#endif
	}
}