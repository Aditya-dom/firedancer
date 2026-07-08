#include "../../util/fd_util.h"
#include "fd_pack_auction.h"
#include "fd_pack_cost.h"

#define TEST_PACK_AUCTION_DEPTH      (64UL)
#define TEST_PACK_AUCTION_BANK_CNT   (2UL)
#define TEST_PACK_AUCTION_SCRATCH_SZ (1UL<<20)

static uchar    test_pack_auction_scratch[ TEST_PACK_AUCTION_SCRATCH_SZ ] __attribute__((aligned(FD_PACK_ALIGN)));
static fd_rng_t test_pack_auction_rng[ 1 ];

static fd_pack_t *
new_test_pack( void ) {
  fd_pack_limits_t limits[ 1 ] = {{
    .max_cost_per_block           = 48000000UL,
    .max_vote_cost_per_block      = 36000000UL,
    .max_write_cost_per_acct      = 12000000UL,
    .max_data_bytes_per_block     = FD_PACK_MAX_DATA_PER_BLOCK,
    .max_txn_per_microblock       = 32UL,
    .max_microblocks_per_block    = 64UL,
    .max_allocated_data_per_block = FD_PACK_MAX_ALLOCATED_DATA_PER_BLOCK,
  }};

  ulong footprint = fd_pack_footprint( TEST_PACK_AUCTION_DEPTH, 0UL, TEST_PACK_AUCTION_BANK_CNT, limits );
  FD_TEST( footprint<=TEST_PACK_AUCTION_SCRATCH_SZ );
  fd_memset( test_pack_auction_scratch, 0, footprint );

  fd_rng_t * rng = fd_rng_join( fd_rng_new( test_pack_auction_rng, 0U, 0UL ) );
  FD_TEST( rng );

  fd_pack_t * pack = fd_pack_join( fd_pack_new( test_pack_auction_scratch,
                                                TEST_PACK_AUCTION_DEPTH,
                                                0UL,
                                                TEST_PACK_AUCTION_BANK_CNT,
                                                limits,
                                                NULL,
                                                0UL,
                                                rng ) );
  FD_TEST( pack );
  return pack;
}

static void
test_schedule_strategy_maps_to_pack_strategy( void ) {
  FD_TEST( fd_pack_strategy_mode_from_schedule_strategy( FD_PACK_STRATEGY_PERF     )==FD_PACK_STRATEGY_LEGACY      );
  FD_TEST( fd_pack_strategy_mode_from_schedule_strategy( FD_PACK_STRATEGY_BALANCED )==FD_PACK_STRATEGY_LEGACY      );
  FD_TEST( fd_pack_strategy_mode_from_schedule_strategy( FD_PACK_STRATEGY_ARAWN    )==FD_PACK_STRATEGY_ARAWN_BATCH );
}

static void
test_arawn_tile_config_populates_pack_config( void ) {
  fd_pack_t * pack = new_test_pack();

  fd_pack_arawn_config_t cfg[ 1 ];
  fd_pack_arawn_config_from_tile( cfg, 50UL );
  fd_pack_set_strategy( pack, fd_pack_strategy_mode_from_schedule_strategy( FD_PACK_STRATEGY_ARAWN ), cfg );

  FD_TEST( fd_pack_get_strategy( pack )==FD_PACK_STRATEGY_ARAWN_BATCH );

  fd_pack_arawn_config_t normalized[ 1 ];
  fd_pack_get_arawn_config( pack, normalized );

  FD_TEST( normalized->auction_period_us==50000UL );
  FD_TEST( normalized->vote_reserve_bps==1200U );
  FD_TEST( normalized->vote_prefix_bps==1500U );
  FD_TEST( normalized->aged_lane_bps==500U );
  FD_TEST( normalized->aged_lane_min_auctions==16UL );
  FD_TEST( normalized->bundle_replace_margin_bps==50U );
  FD_TEST( normalized->frontier_txn_cap==TEST_PACK_AUCTION_DEPTH );
  FD_TEST( normalized->frontier_bundle_cap==64UL );
  FD_TEST( !normalized->jito_compat );
  FD_TEST( normalized->shadow_mode );
}

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
test_arawn_effective_period_uses_base_below_threshold( void ) {
  FD_TEST( fd_pack_arawn_effective_auction_period_ticks_from_backlog( 7UL, 2UL, 80L )==80L );
}

static void
test_arawn_effective_period_halves_at_medium_backlog( void ) {
  FD_TEST( fd_pack_arawn_effective_auction_period_ticks_from_backlog( 8UL, 2UL, 80L )==40L );
  FD_TEST( fd_pack_arawn_effective_auction_period_ticks_from_backlog( 15UL, 2UL, 80L )==40L );
}

static void
test_arawn_effective_period_quarters_at_heavy_backlog( void ) {
  FD_TEST( fd_pack_arawn_effective_auction_period_ticks_from_backlog(  8UL, 1UL, 50L )==13L );
  FD_TEST( fd_pack_arawn_effective_auction_period_ticks_from_backlog( 16UL, 2UL, 80L )==20L );
  FD_TEST( fd_pack_arawn_effective_auction_period_ticks_from_backlog( 32UL, 4UL, 80L )==20L );
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
test_arawn_pack_schedule_advances_pack_auction( void ) {
  fd_pack_t * pack = new_test_pack();
  fd_pack_arawn_config_t cfg[ 1 ];
  fd_pack_arawn_config_from_tile( cfg, 50UL );
  fd_pack_set_strategy( pack, fd_pack_strategy_mode_from_schedule_strategy( FD_PACK_STRATEGY_ARAWN ), cfg );

  long  next_auction_tick = 1000L;
  ulong auction_bank_mask = 0UL;

  int flags = fd_pack_schedule_flags_for_strategy_with_pack( pack,
                                                             FD_PACK_STRATEGY_ARAWN,
                                                             0,
                                                             1,
                                                             999L,
                                                             &next_auction_tick,
                                                             &auction_bank_mask,
                                                             50L );

  FD_TEST( flags & FD_PACK_SCHEDULE_VOTE   );
  FD_TEST( flags & FD_PACK_SCHEDULE_BUNDLE );
  FD_TEST( !(flags & FD_PACK_SCHEDULE_TXN) );
  FD_TEST( fd_pack_current_auction( pack )==0UL );

  flags = fd_pack_schedule_flags_for_strategy_with_pack( pack,
                                                         FD_PACK_STRATEGY_ARAWN,
                                                         0,
                                                         1,
                                                         1000L,
                                                         &next_auction_tick,
                                                         &auction_bank_mask,
                                                         50L );

  FD_TEST( flags & FD_PACK_SCHEDULE_VOTE      );
  FD_TEST( !(flags & FD_PACK_SCHEDULE_BUNDLE) );
  FD_TEST( flags & FD_PACK_SCHEDULE_TXN       );
  FD_TEST( fd_pack_current_auction( pack )==1UL );

  flags = fd_pack_schedule_flags_for_strategy_with_pack( pack,
                                                         FD_PACK_STRATEGY_ARAWN,
                                                         1,
                                                         1,
                                                         1001L,
                                                         &next_auction_tick,
                                                         &auction_bank_mask,
                                                         50L );

  FD_TEST( flags & FD_PACK_SCHEDULE_VOTE      );
  FD_TEST( !(flags & FD_PACK_SCHEDULE_BUNDLE) );
  FD_TEST( flags & FD_PACK_SCHEDULE_TXN       );
  FD_TEST( fd_pack_current_auction( pack )==1UL );

  flags = fd_pack_schedule_flags_for_strategy_with_pack( pack,
                                                         FD_PACK_STRATEGY_ARAWN,
                                                         0,
                                                         1,
                                                         1050L,
                                                         &next_auction_tick,
                                                         &auction_bank_mask,
                                                         50L );

  FD_TEST( flags & FD_PACK_SCHEDULE_VOTE      );
  FD_TEST( !(flags & FD_PACK_SCHEDULE_BUNDLE) );
  FD_TEST( flags & FD_PACK_SCHEDULE_TXN       );
  FD_TEST( fd_pack_current_auction( pack )==2UL );
}

static void
test_arawn_pack_schedule_advances_missed_pack_auctions( void ) {
  fd_pack_t * pack = new_test_pack();
  fd_pack_arawn_config_t cfg[ 1 ];
  fd_pack_arawn_config_from_tile( cfg, 50UL );
  fd_pack_set_strategy( pack, fd_pack_strategy_mode_from_schedule_strategy( FD_PACK_STRATEGY_ARAWN ), cfg );

  long  next_auction_tick = 1000L;
  ulong auction_bank_mask = 0UL;

  int flags = fd_pack_schedule_flags_for_strategy_with_pack( pack,
                                                             FD_PACK_STRATEGY_ARAWN,
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
  FD_TEST( fd_pack_current_auction( pack )==3UL );
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
test_legacy_pack_schedule_does_not_advance_pack_auction( void ) {
  fd_pack_t * pack = new_test_pack();

  long  next_auction_tick = 1000L;
  ulong auction_bank_mask = 0UL;

  int flags = fd_pack_schedule_flags_for_strategy_with_pack( pack,
                                                             FD_PACK_STRATEGY_PERF,
                                                             0,
                                                             1,
                                                             1000L,
                                                             &next_auction_tick,
                                                             &auction_bank_mask,
                                                             50L );

  FD_TEST( flags & FD_PACK_SCHEDULE_VOTE   );
  FD_TEST( flags & FD_PACK_SCHEDULE_BUNDLE );
  FD_TEST( flags & FD_PACK_SCHEDULE_TXN    );
  FD_TEST( next_auction_tick==1000L );
  FD_TEST( !auction_bank_mask );
  FD_TEST( fd_pack_current_auction( pack )==0UL );

  flags = fd_pack_schedule_flags_for_strategy_with_pack( pack,
                                                         FD_PACK_STRATEGY_BALANCED,
                                                         1,
                                                         1,
                                                         1000L,
                                                         &next_auction_tick,
                                                         &auction_bank_mask,
                                                         50L );

  FD_TEST( flags & FD_PACK_SCHEDULE_VOTE      );
  FD_TEST( !(flags & FD_PACK_SCHEDULE_BUNDLE) );
  FD_TEST( !(flags & FD_PACK_SCHEDULE_TXN)    );
  FD_TEST( next_auction_tick==1000L );
  FD_TEST( !auction_bank_mask );
  FD_TEST( fd_pack_current_auction( pack )==0UL );
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

  test_schedule_strategy_maps_to_pack_strategy();
  test_arawn_tile_config_populates_pack_config();
  test_arawn_pack_config_defaults();
  test_arawn_pack_config_normalizes_zero_fields();
  test_arawn_pack_config_preserves_explicit_fields();
  test_arawn_effective_period_uses_base_below_threshold();
  test_arawn_effective_period_halves_at_medium_backlog();
  test_arawn_effective_period_quarters_at_heavy_backlog();
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
  test_arawn_pack_schedule_advances_pack_auction();
  test_arawn_pack_schedule_advances_missed_pack_auctions();
  test_perf_keeps_existing_bundle_gate();
  test_legacy_pack_schedule_does_not_advance_pack_auction();
  test_balanced_keeps_existing_pacing_gate();

  FD_LOG_NOTICE(( "pass" ));
  fd_halt();
  return 0;
}
