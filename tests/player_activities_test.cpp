#include "catch/catch.hpp"
#include "map_helpers.h"
#include "player_helpers.h"
#include "activity_scheduling_helper.h"

#include "activity_actor_definitions.h"
#include "avatar.h"
#include "calendar.h"
#include "character.h"
#include "flag.h"
#include "game.h"
#include "itype.h"
#include "iuse_actor.h"
#include "map.h"
#include "monster.h"
#include "point.h"

static const activity_id ACT_NULL( "ACT_NULL" );
static const activity_id ACT_BOLTCUTTING( "ACT_BOLTCUTTING" );
static const activity_id ACT_SHEARING( "ACT_SHEARING" );

static const efftype_id effect_pet( "pet" );
static const efftype_id effect_tied( "tied" );

static const furn_str_id furn_t_test_f_boltcut1( "test_f_boltcut1" );
static const furn_str_id furn_t_test_f_boltcut2( "test_f_boltcut2" );
static const furn_str_id furn_t_test_f_boltcut3( "test_f_boltcut3" );

static const itype_id itype_test_battery_disposable( "test_battery_disposable" );
static const itype_id itype_test_boltcutter( "test_boltcutter" );
static const itype_id itype_test_boltcutter_elec( "test_boltcutter_elec" );
static const itype_id itype_test_shears( "test_shears" );
static const itype_id itype_test_shears_off( "test_shears_off" );

static const mtype_id mon_test_shearable( "mon_test_shearable" );
static const mtype_id mon_test_non_shearable( "mon_test_non_shearable" );

static const quality_id qual_SHEAR( "SHEAR" );

static const ter_str_id ter_test_t_boltcut1( "test_t_boltcut1" );
static const ter_str_id ter_test_t_boltcut2( "test_t_boltcut2" );

TEST_CASE( "shearing", "[activity][shearing][animals]" )
{
    avatar &dummy = get_avatar();
    clear_avatar();
    clear_map();

    auto test_monster = [&dummy]( bool mon_shearable ) -> monster & {
        monster *mon;
        if( mon_shearable )
        {
            mon = &spawn_test_monster( mon_test_shearable.str(), dummy.pos() + tripoint_north );
        } else
        {
            mon = &spawn_test_monster( mon_test_non_shearable.str(), dummy.pos() + tripoint_north );
        }

        mon->friendly = -1;
        mon->add_effect( effect_pet, 1_turns, true );
        mon->add_effect( effect_tied, 1_turns, true );
        return *mon;
    };


    SECTION( "shearing testing time" ) {

        GIVEN( "player without shearing quality" ) {
            clear_avatar();
            clear_map();
            monster &mon = test_monster( true );

            REQUIRE( dummy.max_quality( qual_SHEAR ) <= 0 );
            dummy.activity = player_activity( shearing_activity_actor( mon.pos(), false ) );
            dummy.activity.start_or_resume( dummy, false );

            THEN( "shearing can't start" ) {
                CHECK( dummy.activity.id() == ACT_NULL );
            }
        }

        GIVEN( "a tool with shearing quality one" ) {
            clear_avatar();
            clear_map();
            monster &mon = test_monster( true );

            dummy.i_add( item( itype_test_shears ) );
            REQUIRE( dummy.max_quality( qual_SHEAR ) == 1 );

            dummy.activity = player_activity( shearing_activity_actor( mon.pos(), false ) );
            dummy.activity.start_or_resume( dummy, false );
            REQUIRE( dummy.activity.id() == ACT_SHEARING );

            THEN( "shearing time is 30 minutes" ) {
                CHECK( dummy.activity.moves_total == to_moves<int>( 30_minutes ) );
            }
        }

        GIVEN( "an electric tool with shearing quality three" ) {
            clear_avatar();
            clear_map();
            monster &mon = test_monster( true );

            item battery( itype_test_battery_disposable );
            battery.ammo_set( battery.ammo_default(), 300 );

            item elec_shears( itype_test_shears_off );
            elec_shears.put_in( battery, item_pocket::pocket_type::MAGAZINE_WELL );

            const use_function *use = elec_shears.type->get_use( "transform" );
            REQUIRE( use != nullptr );
            const iuse_transform *actor = dynamic_cast<const iuse_transform *>( use->get_actor_ptr() );
            actor->use( dummy, elec_shears, false, dummy.pos() );

            dummy.i_add( elec_shears );
            REQUIRE( dummy.max_quality( qual_SHEAR ) == 3 );

            dummy.activity = player_activity( shearing_activity_actor( mon.pos(), false ) );
            dummy.activity.start_or_resume( dummy, false );
            REQUIRE( dummy.activity.id() == ACT_SHEARING );

            THEN( "shearing time is 10 minutes" ) {
                CHECK( dummy.activity.moves_total == to_moves<int>( 10_minutes ) );
            }
        }

        GIVEN( "a non shearable animal" ) {
            clear_avatar();
            clear_map();
            monster &mon = test_monster( false );

            dummy.i_add( item( itype_test_shears ) );
            REQUIRE( dummy.max_quality( qual_SHEAR ) == 1 );

            dummy.activity = player_activity( shearing_activity_actor( mon.pos(), false ) );
            dummy.activity.start_or_resume( dummy, false );

            THEN( "shearing can't start" ) {
                CHECK( dummy.activity.id() == ACT_NULL );
            }
        }
    }

    SECTION( "shearing losing tool" ) {
        GIVEN( "an electric tool with shearing quality three" ) {
            clear_avatar();
            clear_map();
            monster &mon = test_monster( true );

            item battery( itype_test_battery_disposable );
            battery.ammo_set( battery.ammo_default(), 5 );

            item elec_shears( itype_test_shears_off );
            elec_shears.put_in( battery, item_pocket::pocket_type::MAGAZINE_WELL );

            const use_function *use = elec_shears.type->get_use( "transform" );
            REQUIRE( use != nullptr );
            const iuse_transform *actor = dynamic_cast<const iuse_transform *>( use->get_actor_ptr() );
            actor->use( dummy, elec_shears, false, dummy.pos() );

            dummy.i_add( elec_shears );
            REQUIRE( dummy.max_quality( qual_SHEAR ) == 3 );

            dummy.activity = player_activity( shearing_activity_actor( mon.pos(), false ) );
            dummy.activity.start_or_resume( dummy, false );
            REQUIRE( dummy.activity.id() == ACT_SHEARING );

            dummy.moves += dummy.get_speed();
            for( int i = 0; i < 100; ++i ) {
                dummy.activity.do_turn( dummy );
            }

            WHEN( "tool runs out of charges mid activity" ) {
                for( int i = 0; i < 10000; ++i ) {
                    dummy.process_items();
                }

                CHECK( dummy.weapon.ammo_remaining() == 0 );
                REQUIRE( dummy.weapon.typeId().str() == itype_test_shears_off.str() );

                CHECK( dummy.max_quality( qual_SHEAR ) <= 0 );

                THEN( "activity stops without completing" ) {
                    CHECK( dummy.activity.id() == ACT_SHEARING );
                    CHECK( dummy.activity.moves_left > 0 );
                    dummy.activity.do_turn( dummy );
                    CHECK( dummy.activity.id() == ACT_NULL );
                }
            }
        }
    }

    SECTION( "shearing testing result" ) {

        GIVEN( "a shearable monster" ) {

            clear_avatar();
            clear_map();
            monster &mon = test_monster( true );

            dummy.i_add( item( itype_test_shears ) );
            REQUIRE( dummy.max_quality( qual_SHEAR ) == 1 );

            dummy.activity = player_activity( shearing_activity_actor( mon.pos(), false ) );
            dummy.activity.start_or_resume( dummy, false );
            REQUIRE( dummy.activity.id() == ACT_SHEARING );

            const itype_id test_amount( "test_rock" );
            const itype_id test_random( "test_2x4" );
            const itype_id test_mass( "test_rag" );
            const itype_id test_volume( "test_pipe" );

            process_activity( dummy );
            WHEN( "shearing finishes" ) {
                CHECK( dummy.activity.id() == ACT_NULL );
                THEN( "player receives the items" ) {
                    const map_stack items = get_map().i_at( dummy.pos() );
                    int count_amount = 0;
                    int count_random = 0;
                    int count_mass   = 0;
                    int count_volume = 0;
                    for( const item &it : items ) {
                        // can't use switch here
                        const itype_id it_id = it.typeId();
                        if( it_id == test_amount ) {
                            count_amount += it.charges;
                        } else if( it_id == test_random ) {
                            count_random += 1;
                        } else if( it_id == test_mass ) {
                            count_mass   += 1;
                        } else if( it_id == test_volume ) {
                            count_volume += 1;
                        }
                    }

                    CHECK( count_amount == 30 );
                    CHECK( ( 5 <= count_random && count_random <= 10 ) );

                    float weight = units::to_kilogram( mon.get_weight() );
                    CHECK( count_mass == static_cast<int>( 0.5f * weight ) );

                    float volume = units::to_liter( mon.get_volume() );
                    CHECK( count_volume == static_cast<int>( 0.3f * volume ) );
                }
            }
        }

        GIVEN( "an previously tied shearable monster" ) {
            clear_avatar();
            clear_map();
            monster &mon = test_monster( true );

            dummy.i_add( item( itype_test_shears ) );
            REQUIRE( dummy.max_quality( qual_SHEAR ) == 1 );

            dummy.activity = player_activity( shearing_activity_actor( mon.pos(), false ) );
            dummy.activity.start_or_resume( dummy, false );
            REQUIRE( dummy.activity.id() == ACT_SHEARING );

            process_activity( dummy );
            WHEN( "player is done shearing" ) {
                REQUIRE( dummy.activity.id() == ACT_NULL );
                THEN( "monster is tied" ) {
                    CHECK( mon.has_effect( effect_tied ) );
                }
            }
        }

        GIVEN( "a previously untied shearable monster" ) {
            clear_avatar();
            clear_map();
            monster &mon = test_monster( true );

            dummy.i_add( item( itype_test_shears ) );
            REQUIRE( dummy.max_quality( qual_SHEAR ) == 1 );

            dummy.activity = player_activity( shearing_activity_actor( mon.pos(), true ) );
            dummy.activity.start_or_resume( dummy, false );
            REQUIRE( dummy.activity.id() == ACT_SHEARING );

            process_activity( dummy );
            WHEN( "player is done shearing" ) {
                REQUIRE( dummy.activity.id() == ACT_NULL );
                THEN( "monster is untied" ) {
                    CHECK_FALSE( mon.has_effect( effect_tied ) );
                }
            }
        }
    }
}

TEST_CASE( "boltcut", "[activity][boltcut]" )
{
    map &mp = get_map();
    avatar &dummy = get_avatar();

    auto setup_dummy = [&dummy]() -> item_location {
        item it_boltcut( itype_test_boltcutter );

        dummy.wield( it_boltcut );
        REQUIRE( dummy.weapon.typeId() == itype_test_boltcutter );

        return item_location{dummy, &dummy.weapon};
    };

    auto setup_activity = [&dummy]( const item_location & torch ) -> void {
        boltcutting_activity_actor act{tripoint_zero, torch};
        act.testing = true;
        dummy.assign_activity( player_activity( act ) );
    };

    SECTION( "boltcut start checks" ) {
        GIVEN( "a tripoint with nothing" ) {
            clear_map();
            clear_avatar();

            mp.ter_set( tripoint_zero, t_null );
            REQUIRE( mp.ter( tripoint_zero ) == t_null );

            item_location boltcutter = setup_dummy();
            setup_activity( boltcutter );

            THEN( "boltcutting activity can't start" ) {
                CHECK( dummy.activity.id() == ACT_NULL );
            }
        }

        GIVEN( "a tripoint with invalid terrain" ) {
            clear_map();
            clear_avatar();

            mp.ter_set( tripoint_zero, t_dirt );
            REQUIRE( mp.ter( tripoint_zero ) == t_dirt );

            item_location boltcutter = setup_dummy();
            setup_activity( boltcutter );

            THEN( "boltcutting activity can't start" ) {
                CHECK( dummy.activity.id() == ACT_NULL );
            }
        }

        GIVEN( "a tripoint with valid terrain" ) {
            clear_map();
            clear_avatar();

            mp.ter_set( tripoint_zero, ter_test_t_boltcut1 );
            REQUIRE( mp.ter( tripoint_zero ) == ter_test_t_boltcut1 );

            item_location boltcutter = setup_dummy();
            setup_activity( boltcutter );

            THEN( "boltcutting activity can start" ) {
                CHECK( dummy.activity.id() == ACT_BOLTCUTTING );
            }
        }

        GIVEN( "a tripoint with valid furniture" ) {
            clear_map();
            clear_avatar();

            mp.furn_set( tripoint_zero, furn_t_test_f_boltcut1 );
            REQUIRE( mp.furn( tripoint_zero ) == furn_t_test_f_boltcut1 );

            item_location boltcutter = setup_dummy();
            setup_activity( boltcutter );

            THEN( "boltcutting activity can start" ) {
                CHECK( dummy.activity.id() == ACT_BOLTCUTTING );
            }
        }

        GIVEN( "a tripoint with valid terrain" ) {
            clear_map();
            clear_avatar();

            mp.ter_set( tripoint_zero, ter_test_t_boltcut1 );
            REQUIRE( mp.ter( tripoint_zero ) == ter_test_t_boltcut1 );

            item_location boltcutter = setup_dummy();
            setup_activity( boltcutter );
            REQUIRE( dummy.activity.id() == ACT_BOLTCUTTING );

            WHEN( "terrain has a duration of 10 seconds" ) {
                REQUIRE( ter_test_t_boltcut1->boltcut->duration() == 10_seconds );
                THEN( "moves_left is equal to 10 seconds" ) {
                    CHECK( dummy.activity.moves_left == to_moves<int>( 10_seconds ) );
                }
            }
        }

        GIVEN( "a tripoint with valid furniture" ) {
            clear_map();
            clear_avatar();

            mp.furn_set( tripoint_zero, furn_t_test_f_boltcut1 );
            REQUIRE( mp.furn( tripoint_zero ) == furn_t_test_f_boltcut1 );

            item_location boltcutter = setup_dummy();
            setup_activity( boltcutter );
            REQUIRE( dummy.activity.id() == ACT_BOLTCUTTING );

            WHEN( "furniture has a duration of 5 seconds" ) {
                REQUIRE( furn_t_test_f_boltcut1->boltcut->duration() == 5_seconds );
                THEN( "moves_left is equal to 5 seconds" ) {
                    CHECK( dummy.activity.moves_left == to_moves<int>( 5_seconds ) );
                }
            }
        }
    }

    SECTION( "boltcut turn checks" ) {
        GIVEN( "player is in mid activity" ) {
            clear_map();
            clear_avatar();

            mp.furn_set( tripoint_zero, furn_t_test_f_boltcut3 );
            REQUIRE( mp.furn( tripoint_zero ) == furn_t_test_f_boltcut3 );

            item battery( itype_test_battery_disposable );
            battery.ammo_set( battery.ammo_default(), 2 );

            item it_boltcut_elec( itype_test_boltcutter_elec );
            it_boltcut_elec.put_in( battery, item_pocket::pocket_type::MAGAZINE_WELL );

            dummy.wield( it_boltcut_elec );
            REQUIRE( dummy.weapon.typeId() == itype_test_boltcutter_elec );

            item_location boltcutter_elec{dummy, &dummy.weapon};

            setup_activity( boltcutter_elec );
            REQUIRE( dummy.activity.id() == ACT_BOLTCUTTING );
            process_activity( dummy );

            WHEN( "player runs out of charges" ) {
                REQUIRE( dummy.activity.id() == ACT_NULL );

                THEN( "player recharges with fuel" ) {
                    boltcutter_elec->ammo_set( battery.ammo_default() );

                    AND_THEN( "player can resume the activity" ) {
                        setup_activity( boltcutter_elec );
                        dummy.moves = dummy.get_speed();
                        dummy.activity.do_turn( dummy );
                        CHECK( dummy.activity.id() == ACT_BOLTCUTTING );
                        CHECK( dummy.activity.moves_left < to_moves<int>( furn_t_test_f_boltcut3->boltcut->duration() ) );
                    }
                }
            }
        }
    }

    SECTION( "boltcut finish checks" ) {
        GIVEN( "a tripoint with valid terrain" ) {
            clear_map();
            clear_avatar();

            mp.ter_set( tripoint_zero, ter_test_t_boltcut1 );
            REQUIRE( mp.ter( tripoint_zero ) == ter_test_t_boltcut1 );

            item_location boltcutter = setup_dummy();
            setup_activity( boltcutter );

            REQUIRE( dummy.activity.id() == ACT_BOLTCUTTING );
            process_activity( dummy );
            REQUIRE( dummy.activity.id() == ACT_NULL );

            THEN( "terrain gets converted to new terrain type" ) {
                CHECK( mp.ter( tripoint_zero ) == t_dirt );
            }
        }

        GIVEN( "a tripoint with valid furniture" ) {
            clear_map();
            clear_avatar();

            mp.furn_set( tripoint_zero, furn_t_test_f_boltcut1 );
            REQUIRE( mp.furn( tripoint_zero ) == furn_t_test_f_boltcut1 );

            item_location boltcutter = setup_dummy();
            setup_activity( boltcutter );

            REQUIRE( dummy.activity.id() == ACT_BOLTCUTTING );
            process_activity( dummy );
            REQUIRE( dummy.activity.id() == ACT_NULL );

            THEN( "furniture gets converted to new furniture type" ) {
                CHECK( mp.furn( tripoint_zero ) == f_null );
            }
        }

        GIVEN( "a tripoint with valid furniture" ) {
            clear_map();
            clear_avatar();

            mp.furn_set( tripoint_zero, furn_t_test_f_boltcut2 );
            REQUIRE( mp.furn( tripoint_zero ) == furn_t_test_f_boltcut2 );

            item_location boltcutter = setup_dummy();
            setup_activity( boltcutter );

            REQUIRE( dummy.activity.id() == ACT_BOLTCUTTING );
            process_activity( dummy );
            REQUIRE( dummy.activity.id() == ACT_NULL );

            THEN( "furniture gets converted to new furniture type" ) {
                CHECK( mp.furn( tripoint_zero ) == furn_t_test_f_boltcut1 );
            }
        }


        GIVEN( "a tripoint with a valid furniture with byproducts" ) {
            clear_map();
            clear_avatar();

            mp.ter_set( tripoint_zero, ter_test_t_boltcut2 );
            REQUIRE( mp.ter( tripoint_zero ) == ter_test_t_boltcut2 );

            item_location boltcutter = setup_dummy();
            setup_activity( boltcutter );

            REQUIRE( ter_test_t_boltcut2->boltcut->byproducts().size() == 2 );

            REQUIRE( dummy.activity.id() == ACT_BOLTCUTTING );
            process_activity( dummy );
            REQUIRE( dummy.activity.id() == ACT_NULL );

            const itype_id test_amount( "test_rock" );
            const itype_id test_random( "test_2x4" );

            WHEN( "boltcut acitivy finishes" ) {
                CHECK( dummy.activity.id() == ACT_NULL );

                THEN( "player receives the items" ) {
                    const map_stack items = get_map().i_at( tripoint_zero );
                    int count_amount = 0;
                    int count_random = 0;
                    for( const item &it : items ) {
                        // can't use switch here
                        const itype_id it_id = it.typeId();
                        if( it_id == test_amount ) {
                            count_amount += it.charges;
                        } else if( it_id == test_random ) {
                            count_random += 1;
                        }
                    }

                    CHECK( count_amount == 3 );
                    CHECK( ( 7 <= count_random && count_random <= 9 ) );
                }
            }
        }
    }
}
