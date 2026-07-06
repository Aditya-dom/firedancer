#include "../../util/fd_util.h"
#include "fd_pack_auction.h"

static void
test_arawn_blocks_txns_before_auction( void ) {
  long  next_auction_tick = 1000L;
  ulong auction_bank_mask = 0UL;

  int flags = fd_pack_schedule_flags_for_strategy( FD_PACK_STRATEGY_ARAWN,
                                                   0,
                                                   1,
                                                   999L,
                                                   &next_auction_tick,
                                                   &auction_bank_mask,
                                                   50L );

  FD_TEST( flags & FD_PACK_SCHEDULE_VOTE   );
  FD_TEST( flags & FD_PACK_SCHEDULE_BUNDLE );
  FD_TEST( !(flags & FD_PACK_SCHEDULE_TXN) );
  FD_TEST( next_auction_tick==1000L );
  FD_TEST( !auction_bank_mask );
}

static void
test_arawn_opens_txn_batch_at_auction( void ) {
  long  next_auction_tick = 1000L;
  ulong auction_bank_mask = 0UL;

  int flags = fd_pack_schedule_flags_for_strategy( FD_PACK_STRATEGY_ARAWN,
                                                   0,
                                                   1,
                                                   1000L,
                                                   &next_auction_tick,
                                                   &auction_bank_mask,
                                                   50L );

  FD_TEST( flags & FD_PACK_SCHEDULE_VOTE      );
  FD_TEST( !(flags & FD_PACK_SCHEDULE_BUNDLE) );
  FD_TEST( flags & FD_PACK_SCHEDULE_TXN       );
  FD_TEST( next_auction_tick==1050L );
  FD_TEST( auction_bank_mask==1UL );
}

static void
test_arawn_limits_bank_to_one_txn_opportunity_per_auction( void ) {
  long  next_auction_tick = 1050L;
  ulong auction_bank_mask = 1UL;

  int flags = fd_pack_schedule_flags_for_strategy( FD_PACK_STRATEGY_ARAWN,
                                                   0,
                                                   1,
                                                   1001L,
                                                   &next_auction_tick,
                                                   &auction_bank_mask,
                                                   50L );

  FD_TEST( flags & FD_PACK_SCHEDULE_VOTE   );
  FD_TEST( flags & FD_PACK_SCHEDULE_BUNDLE );
  FD_TEST( !(flags & FD_PACK_SCHEDULE_TXN) );
  FD_TEST( next_auction_tick==1050L );
  FD_TEST( auction_bank_mask==1UL );
}

static void
test_arawn_allows_each_bank_once_per_auction( void ) {
  long  next_auction_tick = 1050L;
  ulong auction_bank_mask = 1UL;

  int flags = fd_pack_schedule_flags_for_strategy( FD_PACK_STRATEGY_ARAWN,
                                                   1,
                                                   1,
                                                   1001L,
                                                   &next_auction_tick,
                                                   &auction_bank_mask,
                                                   50L );

  FD_TEST( flags & FD_PACK_SCHEDULE_VOTE      );
  FD_TEST( !(flags & FD_PACK_SCHEDULE_BUNDLE) );
  FD_TEST( flags & FD_PACK_SCHEDULE_TXN       );
  FD_TEST( next_auction_tick==1050L );
  FD_TEST( auction_bank_mask==3UL );
}

static void
test_arawn_resets_bank_mask_next_auction( void ) {
  long  next_auction_tick = 1050L;
  ulong auction_bank_mask = 3UL;

  int flags = fd_pack_schedule_flags_for_strategy( FD_PACK_STRATEGY_ARAWN,
                                                   0,
                                                   1,
                                                   1050L,
                                                   &next_auction_tick,
                                                   &auction_bank_mask,
                                                   50L );

  FD_TEST( flags & FD_PACK_SCHEDULE_VOTE      );
  FD_TEST( !(flags & FD_PACK_SCHEDULE_BUNDLE) );
  FD_TEST( flags & FD_PACK_SCHEDULE_TXN       );
  FD_TEST( next_auction_tick==1100L );
  FD_TEST( auction_bank_mask==1UL );
}

static void
test_arawn_skips_missed_auction_periods( void ) {
  long  next_auction_tick = 1000L;
  ulong auction_bank_mask = 0UL;

  int flags = fd_pack_schedule_flags_for_strategy( FD_PACK_STRATEGY_ARAWN,
                                                   0,
                                                   1,
                                                   1120L,
                                                   &next_auction_tick,
                                                   &auction_bank_mask,
                                                   50L );

  FD_TEST( flags & FD_PACK_SCHEDULE_VOTE      );
  FD_TEST( !(flags & FD_PACK_SCHEDULE_BUNDLE) );
  FD_TEST( flags & FD_PACK_SCHEDULE_TXN       );
  FD_TEST( next_auction_tick==1150L );
  FD_TEST( auction_bank_mask==1UL );
}

static void
test_perf_keeps_existing_bundle_gate( void ) {
  long  next_auction_tick = 1000L;
  ulong auction_bank_mask = 0UL;

  int flags = fd_pack_schedule_flags_for_strategy( FD_PACK_STRATEGY_PERF,
                                                   1,
                                                   0,
                                                   999L,
                                                   &next_auction_tick,
                                                   &auction_bank_mask,
                                                   50L );

  FD_TEST( flags & FD_PACK_SCHEDULE_VOTE   );
  FD_TEST( flags & FD_PACK_SCHEDULE_BUNDLE );
  FD_TEST( flags & FD_PACK_SCHEDULE_TXN    );
}

static void
test_balanced_keeps_existing_pacing_gate( void ) {
  long  next_auction_tick = 1000L;
  ulong auction_bank_mask = 0UL;

  int flags = fd_pack_schedule_flags_for_strategy( FD_PACK_STRATEGY_BALANCED,
                                                   1,
                                                   1,
                                                   1000L,
                                                   &next_auction_tick,
                                                   &auction_bank_mask,
                                                   50L );

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
  test_arawn_limits_bank_to_one_txn_opportunity_per_auction();
  test_arawn_allows_each_bank_once_per_auction();
  test_arawn_resets_bank_mask_next_auction();
  test_arawn_skips_missed_auction_periods();
  test_perf_keeps_existing_bundle_gate();
  test_balanced_keeps_existing_pacing_gate();

  FD_LOG_NOTICE(( "pass" ));
  fd_halt();
  return 0;
}
