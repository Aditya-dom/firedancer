#ifndef HEADER_fd_src_disco_pack_fd_pack_auction_h
#define HEADER_fd_src_disco_pack_fd_pack_auction_h

#include "fd_pack.h"

/* Sync with src/app/shared/fd_config.c */
#define FD_PACK_STRATEGY_PERF     0
#define FD_PACK_STRATEGY_BALANCED 1
#define FD_PACK_STRATEGY_ARAWN    2
#define FD_PACK_STRATEGY_CNT      3

FD_STATIC_ASSERT( FD_PACK_MAX_EXECLE_TILES<=8UL*sizeof(ulong), arawn_bank_mask );

static inline fd_pack_strategy_mode_t
fd_pack_strategy_mode_from_schedule_strategy( int schedule_strategy ) {
  return (schedule_strategy==FD_PACK_STRATEGY_ARAWN) ? FD_PACK_STRATEGY_ARAWN_BATCH : FD_PACK_STRATEGY_LEGACY;
}

static inline void
fd_pack_arawn_config_from_tile( fd_pack_arawn_config_t * cfg,
                                ulong                    auction_period_millis ) {
  fd_pack_arawn_default_config( cfg );
  cfg->auction_period_us = 1000UL * auction_period_millis;
}

static inline ulong
fd_pack_arawn_auction_periods_elapsed( long now,
                                       long next_auction_tick,
                                       long auction_period_ticks ) {
  if( FD_UNLIKELY( auction_period_ticks<=0L ) ) return 0UL;
  if( FD_LIKELY( now<next_auction_tick ) ) return 0UL;

  ulong elapsed_ticks = (ulong)(now - next_auction_tick);
  ulong period_ticks  = (ulong)auction_period_ticks;
  return elapsed_ticks / period_ticks + 1UL;
}

static inline long
fd_pack_arawn_next_auction_tick_after( long now,
                                       long next_auction_tick,
                                       long auction_period_ticks ) {
  if( FD_UNLIKELY( auction_period_ticks<=0L ) ) return now+1L;
  ulong periods = fd_pack_arawn_auction_periods_elapsed( now, next_auction_tick, auction_period_ticks );
  if( FD_LIKELY( !periods ) ) return next_auction_tick;
  ulong period_ticks = (ulong)auction_period_ticks;
  return next_auction_tick + (long)(periods * period_ticks);
}

static inline int
fd_pack_arawn_txn_allowed_ext( long   now,
                               int    bank_idx,
                               long * next_auction_tick,
                               ulong * auction_bank_mask,
                               long   auction_period_ticks,
                               ulong * opened_auction_cnt_out ) {
  if( opened_auction_cnt_out ) *opened_auction_cnt_out = 0UL;
  if( FD_UNLIKELY( auction_period_ticks<=0L ) ) return 0;

  ulong opened_auction_cnt = fd_pack_arawn_auction_periods_elapsed( now, *next_auction_tick, auction_period_ticks );
  if( FD_UNLIKELY( opened_auction_cnt ) ) {
    *next_auction_tick = fd_pack_arawn_next_auction_tick_after( now, *next_auction_tick, auction_period_ticks );
    *auction_bank_mask = 0UL;
  }
  if( opened_auction_cnt_out ) *opened_auction_cnt_out = opened_auction_cnt;
  if( FD_LIKELY( !opened_auction_cnt && now<*next_auction_tick && !*auction_bank_mask ) ) return 0;

  ulong bank_bit = 1UL << (ulong)bank_idx;
  if( FD_UNLIKELY( *auction_bank_mask & bank_bit ) ) return 0;

  *auction_bank_mask |= bank_bit;
  return 1;
}

static inline int
fd_pack_arawn_txn_allowed( long   now,
                           int    bank_idx,
                           long * next_auction_tick,
                           ulong * auction_bank_mask,
                           long   auction_period_ticks ) {
  return fd_pack_arawn_txn_allowed_ext( now,
                                        bank_idx,
                                        next_auction_tick,
                                        auction_bank_mask,
                                        auction_period_ticks,
                                        NULL );
}

static inline int
fd_pack_arawn_select_bank( ulong idle_bank_mask,
                           ulong auction_bank_mask ) {
  ulong unused_idle_bank_mask = fd_ulong_if( !!auction_bank_mask, idle_bank_mask & ~auction_bank_mask, 0UL );
  return fd_ulong_find_lsb( fd_ulong_if( !!unused_idle_bank_mask, unused_idle_bank_mask, idle_bank_mask ) );
}

static inline int
fd_pack_schedule_flags_for_strategy( int    strategy,
                                     int    bank_idx,
                                     int    pacing_execle_cnt,
                                     long   now,
                                     long * next_auction_tick,
                                     ulong * auction_bank_mask,
                                     long   auction_period_ticks ) {
  switch( strategy ) {
    default:
    case FD_PACK_STRATEGY_PERF:
      return FD_PACK_SCHEDULE_VOTE | FD_PACK_SCHEDULE_BUNDLE | FD_PACK_SCHEDULE_TXN;

    case FD_PACK_STRATEGY_BALANCED:
      return FD_PACK_SCHEDULE_VOTE | fd_int_if( bank_idx==0,                FD_PACK_SCHEDULE_BUNDLE, 0 )
                                   | fd_int_if( bank_idx<pacing_execle_cnt, FD_PACK_SCHEDULE_TXN,    0 );

    case FD_PACK_STRATEGY_ARAWN: {
      int allow_txn = fd_pack_arawn_txn_allowed( now,
                                                 bank_idx,
                                                 next_auction_tick,
                                                 auction_bank_mask,
                                                 auction_period_ticks );
      return FD_PACK_SCHEDULE_VOTE | fd_int_if( allow_txn, FD_PACK_SCHEDULE_TXN, 0 )
                                   | fd_int_if( !allow_txn & (bank_idx==0), FD_PACK_SCHEDULE_BUNDLE, 0 );
    }
  }
}

static inline int
fd_pack_schedule_flags_for_strategy_with_pack( fd_pack_t * pack,
                                               int        strategy,
                                               int        bank_idx,
                                               int        pacing_execle_cnt,
                                               long       now,
                                               long *     next_auction_tick,
                                               ulong *    auction_bank_mask,
                                               long       auction_period_ticks ) {
  if( FD_LIKELY( strategy!=FD_PACK_STRATEGY_ARAWN ) ) {
    return fd_pack_schedule_flags_for_strategy( strategy,
                                                bank_idx,
                                                pacing_execle_cnt,
                                                now,
                                                next_auction_tick,
                                                auction_bank_mask,
                                                auction_period_ticks );
  }

  ulong opened_auction_cnt = 0UL;
  int allow_txn = fd_pack_arawn_txn_allowed_ext( now,
                                                 bank_idx,
                                                 next_auction_tick,
                                                 auction_bank_mask,
                                                 auction_period_ticks,
                                                 &opened_auction_cnt );
  if( FD_UNLIKELY( opened_auction_cnt ) ) {
    ulong current_auction = fd_pack_current_auction( pack );
    ulong next_auction    = current_auction + opened_auction_cnt;
    fd_pack_advance_auction( pack, fd_ulong_if( next_auction<current_auction, ULONG_MAX, next_auction ) );
  }

  return FD_PACK_SCHEDULE_VOTE | fd_int_if( allow_txn, FD_PACK_SCHEDULE_TXN, 0 )
                               | fd_int_if( !allow_txn & (bank_idx==0), FD_PACK_SCHEDULE_BUNDLE, 0 );
}

#endif /* HEADER_fd_src_disco_pack_fd_pack_auction_h */
