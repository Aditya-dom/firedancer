#ifndef HEADER_fd_src_disco_pack_fd_pack_auction_h
#define HEADER_fd_src_disco_pack_fd_pack_auction_h

#include "fd_pack.h"

/* Sync with src/app/shared/fd_config.c */
#define FD_PACK_STRATEGY_PERF     0
#define FD_PACK_STRATEGY_BALANCED 1
#define FD_PACK_STRATEGY_ARAWN    2
#define FD_PACK_STRATEGY_CNT      3

FD_STATIC_ASSERT( FD_PACK_MAX_EXECLE_TILES<=8UL*sizeof(ulong), arawn_bank_mask );

static inline long
fd_pack_arawn_next_auction_tick_after( long now,
                                       long next_auction_tick,
                                       long auction_period_ticks ) {
  if( FD_UNLIKELY( auction_period_ticks<=0L ) ) return now+1L;
  if( FD_LIKELY( now<next_auction_tick ) ) return next_auction_tick;

  ulong elapsed_ticks = (ulong)(now - next_auction_tick);
  ulong period_ticks  = (ulong)auction_period_ticks;
  ulong periods       = elapsed_ticks / period_ticks + 1UL;
  return next_auction_tick + (long)(periods * period_ticks);
}

static inline int
fd_pack_arawn_txn_allowed( long   now,
                           int    bank_idx,
                           long * next_auction_tick,
                           ulong * auction_bank_mask,
                           long   auction_period_ticks ) {
  if( FD_UNLIKELY( auction_period_ticks<=0L ) ) return 0;

  int opened_auction = 0;
  if( FD_UNLIKELY( now>=*next_auction_tick ) ) {
    *next_auction_tick = fd_pack_arawn_next_auction_tick_after( now, *next_auction_tick, auction_period_ticks );
    *auction_bank_mask = 0UL;
    opened_auction     = 1;
  }
  if( FD_LIKELY( !opened_auction && now<*next_auction_tick && !*auction_bank_mask ) ) return 0;

  ulong bank_bit = 1UL << (ulong)bank_idx;
  if( FD_UNLIKELY( *auction_bank_mask & bank_bit ) ) return 0;

  *auction_bank_mask |= bank_bit;
  return 1;
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

#endif /* HEADER_fd_src_disco_pack_fd_pack_auction_h */
