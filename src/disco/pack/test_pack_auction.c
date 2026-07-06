#include "../../util/fd_util.h"
#include "fd_pack_auction.h"

static void
test_arawn_blocks_txns_before_auction( void ) {
  long next_auction_tick = 1000L;
  int  auction_active    = 0;

  int flags = fd_pack_schedule_flags_for_strategy( FD_PACK_STRATEGY_ARAWN,
                                                   0,
                                                   1,
                                                   999L,
                                                   &next_auction_tick,
                                                   &auction_active );

  FD_TEST( flags & FD_PACK_SCHEDULE_VOTE   );
  FD_TEST( flags & FD_PACK_SCHEDULE_BUNDLE );
  FD_TEST( !(flags & FD_PACK_SCHEDULE_TXN) );
  FD_TEST( next_auction_tick==1000L );
  FD_TEST( !auction_active );
}

static void
test_arawn_opens_txn_batch_at_auction( void ) {
  long next_auction_tick = 1000L;
  int  auction_active    = 0;

  int flags = fd_pack_schedule_flags_for_strategy( FD_PACK_STRATEGY_ARAWN,
                                                   0,
                                                   1,
                                                   1000L,
                                                   &next_auction_tick,
                                                   &auction_active );

  FD_TEST( flags & FD_PACK_SCHEDULE_VOTE   );
  FD_TEST( flags & FD_PACK_SCHEDULE_BUNDLE );
  FD_TEST( flags & FD_PACK_SCHEDULE_TXN    );
  FD_TEST( next_auction_tick==1000L );
  FD_TEST( auction_active );
}

static void
test_arawn_keeps_batch_open_while_scheduling( void ) {
  fd_txn_e_t txns[1] = {0};

  long next_auction_tick = 1000L;
  int  auction_active    = 1;

  fd_pack_arawn_after_schedule( 1001L,
                                50L,
                                &next_auction_tick,
                                &auction_active,
                                fd_pack_arawn_microblock_has_nonvote_nonbundle( txns, 1UL ) );

  FD_TEST( next_auction_tick==1000L );
  FD_TEST( auction_active );
}

static void
test_arawn_classifies_scheduled_transactions( void ) {
  fd_txn_e_t txns[3] = {0};

  txns[0].txnp->flags = FD_TXN_P_FLAGS_IS_SIMPLE_VOTE;
  FD_TEST( !fd_pack_arawn_microblock_has_nonvote_nonbundle( txns, 1UL ) );

  txns[0].txnp->flags = FD_TXN_P_FLAGS_BUNDLE;
  FD_TEST( !fd_pack_arawn_microblock_has_nonvote_nonbundle( txns, 1UL ) );

  txns[0].txnp->flags = 0U;
  FD_TEST( fd_pack_arawn_microblock_has_nonvote_nonbundle( txns, 1UL ) );

  txns[0].txnp->flags = FD_TXN_P_FLAGS_IS_SIMPLE_VOTE;
  txns[1].txnp->flags = FD_TXN_P_FLAGS_BUNDLE;
  txns[2].txnp->flags = 0U;
  FD_TEST( fd_pack_arawn_microblock_has_nonvote_nonbundle( txns, 3UL ) );
}

static void
test_arawn_closes_batch_after_vote_only_schedule( void ) {
  fd_txn_e_t txns[1] = {0};
  txns[0].txnp->flags = FD_TXN_P_FLAGS_IS_SIMPLE_VOTE;

  long next_auction_tick = 1000L;
  int  auction_active    = 1;

  fd_pack_arawn_after_schedule( 1001L,
                                50L,
                                &next_auction_tick,
                                &auction_active,
                                fd_pack_arawn_microblock_has_nonvote_nonbundle( txns, 1UL ) );

  FD_TEST( next_auction_tick==1050L );
  FD_TEST( !auction_active );
}

static void
test_arawn_closes_empty_batch_and_advances_deadline( void ) {
  long next_auction_tick = 1000L;
  int  auction_active    = 1;

  fd_pack_arawn_after_schedule( 1001L,
                                50L,
                                &next_auction_tick,
                                &auction_active,
                                0 );

  FD_TEST( next_auction_tick==1050L );
  FD_TEST( !auction_active );
}

static void
test_perf_keeps_existing_bundle_gate( void ) {
  long next_auction_tick = 1000L;
  int  auction_active    = 0;

  int flags = fd_pack_schedule_flags_for_strategy( FD_PACK_STRATEGY_PERF,
                                                   1,
                                                   0,
                                                   999L,
                                                   &next_auction_tick,
                                                   &auction_active );

  FD_TEST( flags & FD_PACK_SCHEDULE_VOTE   );
  FD_TEST( flags & FD_PACK_SCHEDULE_BUNDLE );
  FD_TEST( flags & FD_PACK_SCHEDULE_TXN    );
}

static void
test_balanced_keeps_existing_pacing_gate( void ) {
  long next_auction_tick = 1000L;
  int  auction_active    = 0;

  int flags = fd_pack_schedule_flags_for_strategy( FD_PACK_STRATEGY_BALANCED,
                                                   1,
                                                   1,
                                                   1000L,
                                                   &next_auction_tick,
                                                   &auction_active );

  FD_TEST( flags & FD_PACK_SCHEDULE_VOTE      );
  FD_TEST( !(flags & FD_PACK_SCHEDULE_BUNDLE) );
  FD_TEST( !(flags & FD_PACK_SCHEDULE_TXN)    );
}

int
main( int     argc,
      char ** argv ) {
  fd_boot( &argc, &argv );

  test_arawn_blocks_txns_before_auction();
  test_arawn_opens_txn_batch_at_auction();
  test_arawn_keeps_batch_open_while_scheduling();
  test_arawn_classifies_scheduled_transactions();
  test_arawn_closes_batch_after_vote_only_schedule();
  test_arawn_closes_empty_batch_and_advances_deadline();
  test_perf_keeps_existing_bundle_gate();
  test_balanced_keeps_existing_pacing_gate();

  FD_LOG_NOTICE(( "pass" ));
  fd_halt();
  return 0;
}
