#include "includes.h"

Hooks                g_hooks{};;
CustomEntityListener g_custom_entity_listener{};;

Color Hooks::imcolor_to_ccolor(float* col) {
	return Color((int)(col[0] * 255.f), (int)(col[1] * 255.f), (int)(col[2] * 255.f), (int)(col[3] * 255.f));
}

void Pitch_proxy( CRecvProxyData* data, Address ptr, Address out ) {
	// normalize this fucker.
	math::NormalizeAngle( data->m_Value.m_Float );

	// clamp to remove retardedness.
	math::clamp( data->m_Value.m_Float, -90.f, 90.f );

	Player* player = ptr.as< Player* >( );

	if( player ) {
		AimPlayer* rdata = &g_aimbot.m_players[ player->index( ) - 1 ];

		rdata->m_last_angles.x = data->m_Value.m_Float;
	}

	// call original netvar proxy.
	if( g_hooks.m_Pitch_original )
		g_hooks.m_Pitch_original( data, ptr, out );
}

void Yaw_proxy( CRecvProxyData* data, Address ptr, Address out ) {
	// normalize this fucker.
	math::NormalizeAngle( data->m_Value.m_Float );

	// clamp to remove retardedness.
	math::clamp( data->m_Value.m_Float, -180.f, 180.f );

	Player* player = ptr.as< Player* >( );

	if( player ) {
		AimPlayer* rdata = &g_aimbot.m_players[ player->index( ) - 1 ];

		rdata->m_last_angles.y = data->m_Value.m_Float;
	}

	if( g_hooks.m_Yaw_original )
		g_hooks.m_Yaw_original( data, ptr, out );
}

struct stack_frame {
	stack_frame* next;
	DWORD ret;
};

__forceinline DWORD get_ret_addr( int depth = 0 ) {
	stack_frame* fp;

	_asm mov fp, ebp;

	for( int i = 0; i < depth; i++ ) {
		if( !fp )
			break;

		fp = fp->next;
	}

	return fp ? fp->ret : 0;
}

void Body_proxy( CRecvProxyData* data, Address ptr, Address out ) {
	Stack stack;

	static DWORD RecvTable_Decode{ pattern::find( g_csgo.m_engine_dll, XOR( "EB 3F FF 77 10" ) ).as< DWORD >( ) };

	if( RecvTable_Decode != get_ret_addr( 2 ) ) {
		// convert to player.
		Player* player = ptr.as< Player* >( );

		// store data about the update.
		g_resolver.OnBodyUpdate( player, data->m_Value.m_Float );
	}

	// call original proxy.
	if( g_hooks.m_Body_original )
		g_hooks.m_Body_original( data, ptr, out );
}

void SimTime_proxy( CRecvProxyData* data, Address ptr, Address out ) {
	Player* player = ptr.as< Player* >( );

	if( g_hooks.m_SimTime_original )
		g_hooks.m_SimTime_original( data, ptr, out );
}

void AbsYaw_proxy( CRecvProxyData* data, Address ptr, Address out ) {
	// convert to ragdoll.
	//Ragdoll* ragdoll = ptr.as< Ragdoll* >( );

	// get ragdoll owner.
	//Player* player = ragdoll->GetPlayer( );

	// get data for this player.
	/*AimPlayer* aim = &g_aimbot.m_players[ player->index( ) - 1 ];

	if( player && aim ) {
	if( !aim->m_records.empty( ) ) {
	LagRecord* match{ nullptr };

	// iterate records.
	for( const auto &it : aim->m_records ) {
	// find record that matches with simulation time.
	if( it->m_sim_time == player->m_flSimulationTime( ) ) {
	match = it.get( );
	break;
	}
	}

	// we have a match.
	// and it is standing
	// TODO; add air?
	if( match /*&& match->m_mode == Resolver::Modes::RESOLVE_STAND*/// ) {
	/*	RagdollRecord record;
	record.m_record   = match;
	record.m_rotation = math::NormalizedAngle( data->m_Value.m_Float );
	record.m_delta    = math::NormalizedAngle( record.m_rotation - match->m_lbyt );

	float death = math::NormalizedAngle( ragdoll->m_flDeathYaw( ) );

	// store.
	//aim->m_ragdoll.push_front( record );

	//g_cl.print( tfm::format( XOR( "rot %f death %f delta %f\n" ), record.m_rotation, death, record.m_delta ).data( ) );
	}
	}*/
	//}

	// call original netvar proxy.
	if( g_hooks.m_AbsYaw_original )
		g_hooks.m_AbsYaw_original( data, ptr, out );
}

void Force_proxy( CRecvProxyData* data, Address ptr, Address out ) {
	// convert to ragdoll.
	Ragdoll* ragdoll = ptr.as< Ragdoll* >( );

	// get ragdoll owner.
	Player* player = ragdoll->GetPlayer( );

	// we only want this happening to noobs we kill.
	if( g_hooks.b[ XOR( "ragdollForce" ) ] && g_cl.m_local && player && player->enemy( g_cl.m_local ) ) {
		// get m_vecForce.
		vec3_t vel = { data->m_Value.m_Vector[ 0 ], data->m_Value.m_Vector[ 1 ], data->m_Value.m_Vector[ 2 ] };

		// give some speed to all directions.
		vel *= 1000.f;

		// boost z up a bit.
		if( vel.z <= 1.f )
			vel.z = 2.f;

		vel.z *= 2.f;

		// don't want crazy values for this... probably unlikely though?
		math::clamp( vel.x, std::numeric_limits< float >::lowest( ), std::numeric_limits< float >::max( ) );
		math::clamp( vel.y, std::numeric_limits< float >::lowest( ), std::numeric_limits< float >::max( ) );
		math::clamp( vel.z, std::numeric_limits< float >::lowest( ), std::numeric_limits< float >::max( ) );

		// set new velocity.
		data->m_Value.m_Vector[ 0 ] = vel.x;
		data->m_Value.m_Vector[ 1 ] = vel.y;
		data->m_Value.m_Vector[ 2 ] = vel.z;
	}

	if( g_hooks.m_Force_original )
		g_hooks.m_Force_original( data, ptr, out );
}

DECLSPEC_NOINLINE void __fastcall ModifyEyePosition( CCSGOPlayerAnimState* ecx, void* edx, vec3_t& pos ) {
	return;
}

DECLSPEC_NOINLINE bool __fastcall ShouldSkipAnimationFrame( void* ecx, uint32_t* ) {
	return false;
}

DECLSPEC_NOINLINE bool __fastcall SetupBones( void* ECX, void* EDX, matrix3x4_t* pBoneToWorldOut, int nMaxBones, int boneMask, float currentTime ) {
	Player* player = ( Player* )( uintptr_t( ECX ) - 4 );

	if( player->index( ) > 64 || player->index( ) < 0 )
		return ( ( Hooks::SetupBones_t )g_hooks.m_SetupBones )( ECX, pBoneToWorldOut, nMaxBones, boneMask, currentTime );

	if( !g_cl.m_updating_bones ) {
		if( !pBoneToWorldOut || nMaxBones <= 0 )
			return true;

		if( pBoneToWorldOut ) {
			memcpy( pBoneToWorldOut, player->m_BoneCache( ).m_pCachedBones, sizeof( matrix3x4a_t ) * std::min( nMaxBones, player->m_BoneCache( ).m_CachedBoneCount ) );
			return true;
		}

		return false;
	}

	for( int i = 0; i < player->m_AnimOverlayCount( ); ++i ) {
		auto& elem = player->m_AnimOverlay( )[ i ];

		if( player != elem.m_owner )
			elem.m_owner = player;
	}

	auto prev_p = ( player->m_PlayerAnimState( ) == nullptr ) ? 0 : player->m_PlayerAnimState( )->m_weapon_last_bone_setup;

	if( player->m_PlayerAnimState( ) != nullptr )
		player->m_PlayerAnimState( )->m_weapon_last_bone_setup = player->m_PlayerAnimState( )->m_weapon;

	auto og = ( ( Hooks::SetupBones_t )g_hooks.m_SetupBones )( ECX, pBoneToWorldOut, nMaxBones, boneMask, currentTime );

	if( player->m_PlayerAnimState( ) != nullptr )
		player->m_PlayerAnimState( )->m_weapon_last_bone_setup = prev_p;

	return og;
}
#pragma optimize( "", off ) // ���� ��������� ��� � �� �� ������������� �� ����

void Hooks::init( ) {
	VMPBSTART("hooksInit");

	// hook wndproc.
	m_old_wndproc = ( WNDPROC )g_winapi.SetWindowLongA( g_csgo.m_game->m_hWindow, GWL_WNDPROC, util::force_cast< LONG >( Hooks::WndProc ) );

	// setup normal VMT hooks.
	m_panel.init( g_csgo.m_panel );
	m_panel.add( IPanel::PAINTTRAVERSE, util::force_cast( &Hooks::PaintTraverse ) );

	m_client.init( g_csgo.m_client );
	//oDispatchUserMessage = m_client.add< tDispatchUserMessage >( CHLClient::DISPATCHUSERMESSAGE, util::force_cast( &hkDispatchUserMessage ) );
	m_client.add( CHLClient::LEVELINITPREENTITY, util::force_cast( &Hooks::LevelInitPreEntity ) );
	m_client.add( CHLClient::LEVELINITPOSTENTITY, util::force_cast( &Hooks::LevelInitPostEntity ) );
	m_client.add( CHLClient::LEVELSHUTDOWN, util::force_cast( &Hooks::LevelShutdown ) );
	//m_client.add( CHLClient::INKEYEVENT, util::force_cast( &Hooks::IN_KeyEvent ) );
	m_client.add( CHLClient::FRAMESTAGENOTIFY, util::force_cast( &Hooks::FrameStageNotify ) );
	m_client.add( CHLClient::USRCMDTODELTABUFFER, util::force_cast( &Hooks::WriteUsercmdDeltaToBuffer ) );

	m_engine.init( g_csgo.m_engine );
	m_engine.add( IVEngineClient::ISCONNECTED, util::force_cast( &Hooks::IsConnected ) );
	m_engine.add( IVEngineClient::ISHLTV, util::force_cast( &Hooks::IsHLTV ) );
	m_engine.add( IVEngineClient::ISPAUSED, util::force_cast( &Hooks::IsPaused ) );

	m_prediction.init( g_csgo.m_prediction );
	m_prediction.add( CPrediction::INPREDICTION, util::force_cast( &Hooks::InPrediction ) );
	m_prediction.add( CPrediction::RUNCOMMAND, util::force_cast( &Hooks::RunCommand ) );

	m_client_mode.init( g_csgo.m_client_mode );
	m_client_mode.add( IClientMode::SHOULDDRAWPARTICLES, util::force_cast( &Hooks::ShouldDrawParticles ) );
	m_client_mode.add( IClientMode::SHOULDDRAWFOG, util::force_cast( &Hooks::ShouldDrawFog ) );
	m_client_mode.add( IClientMode::OVERRIDEVIEW, util::force_cast( &Hooks::OverrideView ) );
	m_client_mode.add( IClientMode::CREATEMOVE, util::force_cast( &Hooks::CreateMove ) );
	m_client_mode.add( IClientMode::DOPOSTSPACESCREENEFFECTS, util::force_cast( &Hooks::DoPostScreenSpaceEffects ) );

	m_surface.init( g_csgo.m_surface );
	//m_surface.add( ISurface::GETSCREENSIZE, util::force_cast( &Hooks::GetScreenSize ) );
	m_surface.add( ISurface::LOCKCURSOR, util::force_cast( &Hooks::LockCursor ) );
	m_surface.add( ISurface::PLAYSOUND, util::force_cast( &Hooks::PlaySound ) );
	m_surface.add( ISurface::ONSCREENSIZECHANGED, util::force_cast( &Hooks::OnScreenSizeChanged ) );

	m_model_render.init( g_csgo.m_model_render );
	m_model_render.add( IVModelRender::DRAWMODELEXECUTE, util::force_cast( &Hooks::DrawModelExecute ) );

	m_render_view.init( g_csgo.m_render_view );
	m_render_view.add( IVRenderView::SCENEEND, util::force_cast( &Hooks::SceneEnd ) );

	m_shadow_mgr.init( g_csgo.m_shadow_mgr );
	m_shadow_mgr.add( IClientShadowMgr::COMPUTESHADOWDEPTHTEXTURES, util::force_cast( &Hooks::ComputeShadowDepthTextures ) );

	m_view_render.init( g_csgo.m_view_render );
	m_view_render.add( CViewRender::ONRENDERSTART, util::force_cast( &Hooks::OnRenderStart ) );
	m_view_render.add( CViewRender::RENDERVIEW, util::force_cast( &Hooks::RenderView ) );
	m_view_render.add( CViewRender::RENDER2DEFFECTSPOSTHUD, util::force_cast( &Hooks::Render2DEffectsPostHUD ) );
	m_view_render.add( CViewRender::RENDERSMOKEOVERLAY, util::force_cast( &Hooks::RenderSmokeOverlay ) );

	m_match_framework.init( g_csgo.m_match_framework );
	m_match_framework.add( CMatchFramework::GETMATCHSESSION, util::force_cast( &Hooks::GetMatchSession ) );

	m_material_system.init( g_csgo.m_material_system );
	m_material_system.add( IMaterialSystem::OVERRIDECONFIG, util::force_cast( &Hooks::OverrideConfig ) );

	m_fire_bullets.init( g_csgo.TEFireBullets );
	m_fire_bullets.add( 7, util::force_cast( &Hooks::PostDataUpdate ) );

	auto setupBones = ( DWORD )pattern::find( g_csgo.m_client_dll, XOR( "55 8B EC 83 E4 F0 B8 D8" ) );
	m_SetupBones = ( DWORD )DetourFunction( ( byte* )setupBones, ( byte* )SetupBones );

	auto modifyEyePos = ( DWORD )pattern::find( g_csgo.m_client_dll, XOR( "55 8B EC 83 E4 F8 83 EC 58 56 57 8B F9 83 7F 60 00 " ) );
	( DWORD )DetourFunction( ( byte* )modifyEyePos, ( byte* )ModifyEyePosition );

	auto shouldSkipAnimFrame = ( DWORD )pattern::find( g_csgo.m_client_dll, XOR( "E8 ? ? ? ? 88 44 24 0B" ) ).rel32( 1 );
	( DWORD )DetourFunction( ( byte* )shouldSkipAnimFrame, ( byte* )ShouldSkipAnimationFrame );

	// register our custom entity listener.
	// todo - dex; should we push our listeners first? should be fine like this.
	g_custom_entity_listener.init( );

	m_device.init( g_csgo.m_device );
	m_device.add( 17, util::force_cast( &Hooks::Present ) );
	m_device.add( 16, util::force_cast( &Hooks::Reset ) );

	// cvar.
	m_net_show_fragments.init( g_csgo.net_showfragments );
	m_net_show_fragments.add( ConVar::GETBOOL, util::force_cast( &Hooks::NetShowFragmentsGetBool ) );

	// set netvar proxies.
	g_netvars.SetProxy( HASH( "DT_CSPlayer" ), HASH( "m_angEyeAngles[0]" ), Pitch_proxy, m_Pitch_original );
	g_netvars.SetProxy( HASH( "DT_CSPlayer" ), HASH( "m_angEyeAngles[1]" ), Yaw_proxy, m_Yaw_original );
	g_netvars.SetProxy( HASH( "DT_CSPlayer" ), HASH( "m_flLowerBodyYawTarget" ), Body_proxy, m_Body_original );
	g_netvars.SetProxy( HASH( "DT_CSRagdoll" ), HASH( "m_vecForce" ), Force_proxy, m_Force_original );
	g_netvars.SetProxy( HASH( "DT_CSRagdoll" ), HASH( "m_flAbsYaw" ), AbsYaw_proxy, m_AbsYaw_original );
	g_netvars.SetProxy( HASH( "DT_BaseEntity" ), HASH( "m_flSimulationTime" ), SimTime_proxy, m_SimTime_original );

	//testJS();


	static bool unlocked_fakelag = false;
	if( !unlocked_fakelag ) {
		auto cl_move_clamp = pattern::find( g_csgo.m_engine_dll, XOR( "B8 ? ? ? ? 3B F0 0F 4F F0 89 5D FC" ) ) + 1;
		unsigned long protect = 0;

		VirtualProtect( ( void* )cl_move_clamp, 4, PAGE_EXECUTE_READWRITE, &protect );
		*( std::uint32_t* )cl_move_clamp = 62;
		VirtualProtect( ( void* )cl_move_clamp, 4, protect, &protect );
		unlocked_fakelag = true;
	}
	VMPEND();
}
#pragma optimize( "", on ) // ���� ��������� ��� � �� �� ������������� �� ����