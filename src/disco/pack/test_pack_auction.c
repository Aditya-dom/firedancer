#include "../../util/fd_util.h"
#include "fd_pack_auction.h"

static void
test_arawn_pack_config_defaults( void ) {
  fd_pack_arawn_config_t cfg[1];
  fd_pack_arawn_default_config( cfg );

  FD_TEST( cfg->auction_period_us==8000UL );
  FD_TEST( cfg->vote_reserve_bps==1200U );
  FD_TEST( cfg->vote_prefix_bps==1500U );
  FD_TEST( cfg->aged_lane_bps==500U );
  FD_TEST( cfg->aged_lane_min_auctions==16UL );
  FD_TEST( cfg->bundle_replace_margin_bps==50U );
  FD_TEST( cfg->frontier_txn_cap==0UL );
  FD_TEST( cfg->frontier_bundle_cap==64UL );
  FD_TEST( !cfg->jito_compat );
  FD_TEST( cfg->shadow_mode );
}

static void
test_arawn_pack_config_normalizes_zero_fields( void ) {
  fd_pack_arawn_config_t cfg = {0};

  fd_pack_arawn_normalize_config( &cfg, 4096UL );

  FD_TEST( cfg.auction_period_us==8000UL );
  FD_TEST( cfg.vote_reserve_bps==1200U );
  FD_TEST( cfg.vote_prefix_bps==1500U );
  FD_TEST( cfg.aged_lane_bps==500U );
  FD_TEST( cfg.aged_lane_min_auctions==16UL );
  FD_TEST( cfg.bundle_replace_margin_bps==50U );
  FD_TEST( cfg.frontier_txn_cap==1024UL );
  FD_TEST( cfg.frontier_bundle_cap==64UL );
  FD_TEST( !cfg.jito_compat );
  FD_TEST( !cfg.shadow_mode );
}

static void
test_arawn_pack_config_preserves_explicit_fields( void ) {
  fd_pack_arawn_config_t cfg = {0};
  cfg.auction_period_us          = 12000UL;
  cfg.vote_reserve_bps           = 1000U;
  cfg.vote_prefix_bps            = 500U;
  cfg.aged_lane_bps              = 250U;
  cfg.aged_lane_min_auctions     = 8UL;
  cfg.bundle_replace_margin_bps  = 25U;
  cfg.frontier_txn_cap           = 32UL;
  cfg.frontier_bundle_cap        = 16UL;
  cfg.jito_compat                = 1;
  cfg.shadow_mode                = 0;

  fd_pack_arawn_normalize_config( &cfg, 4096UL );

  FD_TEST( cfg.auction_period_us==12000UL );
  FD_TEST( cfg.vote_reserve_bps==1000U );
  FD_TEST( cfg.vote_prefix_bps==500U );
  FD_TEST( cfg.aged_lane_bps==250U );
  FD_TEST( cfg.aged_lane_min_auctions==8UL );
  FD_TEST( cfg.bundle_replace_margin_bps==25U );
  FD_TEST( cfg.frontier_txn_cap==32UL );
  FD_TEST( cfg.frontier_bundle_cap==16UL );
  FD_TEST( cfg.jito_compat );
  FD_TEST( !cfg.shadow_mode );
}

static void
test_arawn_pack_state_advances_monotonically( void ) {
  fd_pack_arawn_state_t state[1];
  fd_pack_arawn_state_init( state, 4096UL );

  FD_TEST( fd_pack_arawn_state_strategy( state )==FD_PACK_STRATEGY_LEGACY );
  FD_TEST( fd_pack_arawn_state_current_auction( state )==0UL );
  FD_TEST( fd_pack_arawn_state_advance_auction( state, 7UL )==1 );
  FD_TEST( fd_pack_arawn_state_current_auction( state )==7UL );
  FD_TEST( fd_pack_arawn_state_advance_auction( state, 6UL )==0 );
  FD_TEST( fd_pack_arawn_state_current_auction( state )==7UL );
  FD_TEST( fd_pack_arawn_state_advance_auction( state, 7UL )==0 );
  FD_TEST( fd_pack_arawn_state_current_auction( state )==7UL );
}

static void
test_arawn_pack_state_assigns_arrival_sequences( void ) {
  fd_pack_arawn_state_t state[1];
  fd_pack_arawn_state_init( state, 4096UL );

  FD_TEST( fd_pack_arawn_state_next_arrival_seq( state )==0UL );
  FD_TEST( fd_pack_arawn_state_next_arrival_seq( state )==1UL );
  FD_TEST( fd_pack_arawn_state_next_arrival_seq( state )==2UL );
}

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
test_arawn_selects_unused_idle_bank_during_batch( void ) {
  FD_TEST( fd_pack_arawn_select_bank( 3UL, 1UL )==1 );
  FD_TEST( fd_pack_arawn_select_bank( 7UL, 3UL )==2 );
}

static void
test_arawn_selects_lowest_idle_bank_without_unused_ready_bank( void ) {
  FD_TEST( fd_pack_arawn_select_bank( 1UL, 0UL )==0 );
  FD_TEST( fd_pack_arawn_select_bank( 1UL, 1UL )==0 );
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

  test_arawn_pack_config_defaults();
  test_arawn_pack_config_normalizes_zero_fields();
  test_arawn_pack_config_preserves_explicit_fields();
  test_arawn_pack_state_advances_monotonically();
  test_arawn_pack_state_assigns_arrival_sequences();
  test_arawn_blocks_txns_before_auction();
  test_arawn_opens_txn_batch_at_auction();
  test_arawn_limits_bank_to_one_txn_opportunity_per_auction();
  test_arawn_allows_each_bank_once_per_auction();
  test_arawn_resets_bank_mask_next_auction();
  test_arawn_skips_missed_auction_periods();
  test_arawn_selects_unused_idle_bank_during_batch();
  test_arawn_selects_lowest_idle_bank_without_unused_ready_bank();
  test_perf_keeps_existing_bundle_gate();
  test_balanced_keeps_existing_pacing_gate();

  FD_LOG_NOTICE(( "pass" ));
  fd_halt();
  return 0;
}
