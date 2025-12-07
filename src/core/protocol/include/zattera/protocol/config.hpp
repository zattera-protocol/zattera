/*
 * Copyright (c) 2025 Zattera Foundation, and contributors.
 */
#pragma once
#include <zattera/protocol/hardfork.hpp>

// WARNING!
// Every symbol defined here needs to be handled appropriately in get_config.cpp
// This is checked by get_config_check.sh called from Dockerfile

#ifdef IS_TEST_NET
#define ZATTERA_BLOCKCHAIN_VERSION              ( version(0, 0, 0) )

#define ZATTERA_GENESIS_PRIVATE_KEY             (fc::ecc::private_key::regenerate(fc::sha256::hash(std::string("init_key"))))
#define ZATTERA_GENESIS_PUBLIC_KEY_STR          (std::string( zattera::protocol::public_key_type(ZATTERA_GENESIS_PRIVATE_KEY.get_public_key()) ))
#define ZATTERA_CHAIN_ID_NAME                   "testnet"
#define ZATTERA_CHAIN_ID                        (fc::sha256::hash(ZATTERA_CHAIN_ID_NAME))
#define ZATTERA_ADDRESS_PREFIX                  "TST"

#define ZATTERA_GENESIS_TIME                    (fc::time_point_sec(1451606400))
#define ZATTERA_CASHOUT_WINDOW_SECONDS          (60*60*24*1) // 1 day
#define ZATTERA_UPVOTE_LOCKOUT                  (fc::hours(12))

#define ZATTERA_MIN_ACCOUNT_CREATION_FEE        0

#define ZATTERA_OWNER_AUTH_RECOVERY_PERIOD                  fc::seconds(60)
#define ZATTERA_ACCOUNT_RECOVERY_REQUEST_EXPIRATION_PERIOD  fc::seconds(12)
#define ZATTERA_OWNER_UPDATE_LIMIT                          fc::seconds(0)
#define ZATTERA_OWNER_AUTH_HISTORY_TRACKING_START_BLOCK_NUM 1

#define ZATTERA_INITIAL_SUPPLY                  (int64_t( 250 ) * int64_t( 1000000 ) * int64_t( 1000 ))
#define ZATTERA_ZBD_INITIAL_SUPPLY              (int64_t( 2 ) * int64_t( 1000000 ) * int64_t( 1000 ))

/// Allows to limit number of total produced blocks.
#define TESTNET_BLOCK_LIMIT                   (3000000)

#else // IS LIVE ZATTERA NETWORK

#define ZATTERA_BLOCKCHAIN_VERSION              ( version(0, 0, 0) )

#define ZATTERA_GENESIS_PUBLIC_KEY_STR          "ZTR8GC13uCZbP44HzMLV6zPZGwVQ8Nt4Kji8PapsPiNq1BK153XTX"
#define ZATTERA_CHAIN_ID_NAME                   ""
#define ZATTERA_CHAIN_ID                        fc::sha256()
#define ZATTERA_ADDRESS_PREFIX                  "ZTR"

#define ZATTERA_GENESIS_TIME                    (fc::time_point_sec(1458835200))
#define ZATTERA_CASHOUT_WINDOW_SECONDS          (60*60*24*7)  // 7 days
#define ZATTERA_UPVOTE_LOCKOUT                  (fc::hours(12))

#define ZATTERA_MIN_ACCOUNT_CREATION_FEE        1

#define ZATTERA_OWNER_AUTH_RECOVERY_PERIOD                  fc::days(30)
#define ZATTERA_ACCOUNT_RECOVERY_REQUEST_EXPIRATION_PERIOD  fc::days(1)
#define ZATTERA_OWNER_UPDATE_LIMIT                          fc::minutes(60)
#define ZATTERA_OWNER_AUTH_HISTORY_TRACKING_START_BLOCK_NUM 3186477

#define ZATTERA_INITIAL_SUPPLY                  int64_t(0)
#define ZATTERA_ZBD_INITIAL_SUPPLY              int64_t(0)

#endif

#define VESTS_SYMBOL  (zattera::protocol::asset_symbol_type::from_asset_num( ZATTERA_ASSET_NUM_VESTS ) )
#define ZTR_SYMBOL    (zattera::protocol::asset_symbol_type::from_asset_num( ZATTERA_ASSET_NUM_ZTR ) )
#define ZBD_SYMBOL    (zattera::protocol::asset_symbol_type::from_asset_num( ZATTERA_ASSET_NUM_ZBD ) )

#define ZATTERA_BLOCKCHAIN_HARDFORK_VERSION     ( hardfork_version( ZATTERA_BLOCKCHAIN_VERSION ) )

#define ZATTERA_BLOCK_INTERVAL                  3
#define ZATTERA_BLOCKS_PER_YEAR                 (365*24*60*60/ZATTERA_BLOCK_INTERVAL)
#define ZATTERA_BLOCKS_PER_DAY                  (24*60*60/ZATTERA_BLOCK_INTERVAL)
#define ZATTERA_START_VESTING_BLOCK             (ZATTERA_BLOCKS_PER_DAY * 7)
#define ZATTERA_START_WITNESS_VOTING_BLOCK      (ZATTERA_BLOCKS_PER_DAY * 7)

#define ZATTERA_GENESIS_WITNESS_NAME            "genesis"
#define ZATTERA_NUM_GENESIS_WITNESSES           1
#define ZATTERA_INIT_TIME                       (fc::time_point_sec());

#define ZATTERA_MAX_WITNESSES                   21

#define ZATTERA_MAX_VOTED_WITNESSES             20
#define ZATTERA_MAX_RUNNER_WITNESSES            1

#define ZATTERA_HARDFORK_REQUIRED_WITNESSES     17 // 17 of the 21 dpos witnesses (20 elected and 1 virtual time) required for hardfork. This guarantees 75% participation on all subsequent rounds.
#define ZATTERA_MAX_TIME_UNTIL_EXPIRATION       (60*60) // seconds,  aka: 1 hour
#define ZATTERA_MAX_MEMO_SIZE                   2048
#define ZATTERA_MAX_PROXY_RECURSION_DEPTH       4
#define ZATTERA_VESTING_WITHDRAW_INTERVALS      13
#define ZATTERA_VESTING_WITHDRAW_INTERVAL_SECONDS (60*60*24*7) // 1 week per interval
#define ZATTERA_MAX_WITHDRAW_ROUTES             10
#define ZATTERA_SAVINGS_WITHDRAW_TIME           (fc::days(3))
#define ZATTERA_SAVINGS_WITHDRAW_REQUEST_LIMIT  100
#define ZATTERA_VOTE_REGENERATION_SECONDS       (5*60*60*24) // 5 day
#define ZATTERA_MAX_VOTE_CHANGES                5
#define ZATTERA_REVERSE_AUCTION_WINDOW_SECONDS  (60*30) // 30 minutes
#define ZATTERA_MIN_VOTE_INTERVAL_SEC           3
#define ZATTERA_MIN_VOTE_VESTING_SHARES         (1000000)
#define ZATTERA_VOTE_DUST_THRESHOLD             (1000)

#define ZATTERA_MIN_ROOT_COMMENT_INTERVAL       (fc::seconds(60*5)) // 5 minutes
#define ZATTERA_MIN_REPLY_INTERVAL              (fc::seconds(3)) // 3 seconds
#define ZATTERA_POST_AVERAGE_WINDOW             (60*60*24u) // 1 day
#define ZATTERA_POST_WEIGHT_CONSTANT            (uint64_t(4*ZATTERA_100_PERCENT) * (4*ZATTERA_100_PERCENT)) // (4*ZATTERA_100_PERCENT) -> 2 posts per 1 days, average 1 every 12 hours

#define ZATTERA_MAX_ACCOUNT_WITNESS_VOTES       30

#define ZATTERA_100_PERCENT                     10000
#define ZATTERA_1_PERCENT                       (ZATTERA_100_PERCENT/100)
#define ZATTERA_DEFAULT_ZBD_INTEREST_RATE       (10*ZATTERA_1_PERCENT) // < 10% APR

#define ZATTERA_INFLATION_RATE_START_PERCENT    (978) // Fixes block 7,000,000 to 9.5%
#define ZATTERA_INFLATION_RATE_STOP_PERCENT     (95) // 0.95%
#define ZATTERA_INFLATION_NARROWING_PERIOD      (250000) // Narrow 0.01% every 250k blocks
#define ZATTERA_CONTENT_REWARD_PERCENT          (75*ZATTERA_1_PERCENT) // 75% of inflation, 7.125% inflation
#define ZATTERA_VESTING_FUND_PERCENT            (15*ZATTERA_1_PERCENT) // 15% of inflation, 1.425% inflation
#define ZATTERA_MIN_REWARD_VESTING_SHARES       int64_t(1000000000000ll) // minimum VESTS to start distributing vesting rewards. (1,000,000.000000 VESTS)

#define ZATTERA_BOOTSTRAP_FIXED_BLOCK_REWARD    (10000) // 10.000 ZTR per block (3 decimal precision)
#define ZATTERA_BOOTSTRAP_SUPPLY_THRESHOLD      int64_t(100000000000ll) // 100,000,000.000 ZTR threshold

#define ZATTERA_MAX_RATION_DECAY_RATE           (1000000)

#define ZATTERA_BANDWIDTH_AVERAGE_WINDOW_SECONDS (60*60*24*7) // < 1 week
#define ZATTERA_BANDWIDTH_PRECISION             (uint64_t(1000000)) // < 1 million
#define ZATTERA_MAX_COMMENT_DEPTH               0xffff // 64k
#define ZATTERA_SOFT_MAX_COMMENT_DEPTH          0xff // 255

#define ZATTERA_MAX_RESERVE_RATIO               (20000)

#define ZATTERA_CREATE_ACCOUNT_WITH_ZATTERA_MODIFIER 30
#define ZATTERA_CREATE_ACCOUNT_DELEGATION_RATIO    5
#define ZATTERA_CREATE_ACCOUNT_DELEGATION_TIME     fc::days(30)

#define ZATTERA_ACTIVE_CHALLENGE_FEE            asset( 2000, ZTR_SYMBOL )
#define ZATTERA_OWNER_CHALLENGE_FEE             asset( 30000, ZTR_SYMBOL )
#define ZATTERA_ACTIVE_CHALLENGE_COOLDOWN       fc::days(1)
#define ZATTERA_OWNER_CHALLENGE_COOLDOWN        fc::days(1)

#define ZATTERA_POST_REWARD_FUND_NAME           ("post")
#define ZATTERA_COMMENT_REWARD_FUND_NAME        ("comment")
#define ZATTERA_RECENT_RSHARES_DECAY_TIME       (fc::days(15))
#define ZATTERA_CONTENT_CONSTANT                (uint128_t(uint64_t(2000000000000ll)))

#define ZATTERA_MIN_PAYOUT_ZBD                  (asset(20,ZBD_SYMBOL))

#define ZATTERA_ZBD_STOP_PERCENT                (5*ZATTERA_1_PERCENT ) // Stop printing ZBD at 5% Market Cap
#define ZATTERA_ZBD_START_PERCENT               (2*ZATTERA_1_PERCENT) // Start reducing printing of ZBD at 2% Market Cap

#define ZATTERA_MIN_ACCOUNT_NAME_LENGTH          3
#define ZATTERA_MAX_ACCOUNT_NAME_LENGTH         16

#define ZATTERA_MIN_PERMLINK_LENGTH             0
#define ZATTERA_MAX_PERMLINK_LENGTH             256
#define ZATTERA_MAX_WITNESS_URL_LENGTH          2048

#define ZATTERA_MAX_SHARE_SUPPLY                int64_t(1000000000000000ll)
#define ZATTERA_MAX_SATOSHIS                    int64_t(4611686018427387903ll)
#define ZATTERA_MAX_SIG_CHECK_DEPTH             2
#define ZATTERA_MAX_SIG_CHECK_ACCOUNTS          125

#define ZATTERA_MIN_TRANSACTION_SIZE_LIMIT      1024
#define ZATTERA_SECONDS_PER_YEAR                (uint64_t(60*60*24*365ll))

#define ZATTERA_ZBD_INTEREST_COMPOUND_INTERVAL_SEC  (60*60*24*30)
#define ZATTERA_MAX_TRANSACTION_SIZE            (1024*64)
#define ZATTERA_MIN_BLOCK_SIZE_LIMIT            (ZATTERA_MAX_TRANSACTION_SIZE)
#define ZATTERA_MAX_BLOCK_SIZE                  (ZATTERA_MAX_TRANSACTION_SIZE*ZATTERA_BLOCK_INTERVAL*2000)
#define ZATTERA_SOFT_MAX_BLOCK_SIZE             (2*1024*1024)
#define ZATTERA_MIN_BLOCK_SIZE                  115
#define ZATTERA_BLOCKS_PER_HOUR                 (60*60/ZATTERA_BLOCK_INTERVAL)
#define ZATTERA_FEED_INTERVAL_BLOCKS            (ZATTERA_BLOCKS_PER_HOUR)
#define ZATTERA_FEED_HISTORY_WINDOW             (12*7) // 3.5 days
#define ZATTERA_MAX_FEED_AGE_SECONDS            (60*60*24*7) // 7 days
#define ZATTERA_MIN_FEEDS                       (ZATTERA_MAX_WITNESSES/3) // protects the network from conversions before price has been established
#define ZATTERA_CONVERSION_DELAY                (fc::hours(ZATTERA_FEED_HISTORY_WINDOW)) // 3.5 day conversion

#define ZATTERA_MIN_UNDO_HISTORY                10
#define ZATTERA_MAX_UNDO_HISTORY                10000

#define ZATTERA_MIN_TRANSACTION_EXPIRATION_LIMIT (ZATTERA_BLOCK_INTERVAL * 5) // 5 transactions per block
#define ZATTERA_BLOCKCHAIN_PRECISION            uint64_t( 1000 )

#define ZATTERA_BLOCKCHAIN_PRECISION_DIGITS     3
#define ZATTERA_MAX_INSTANCE_ID                 (uint64_t(-1)>>16)
/** NOTE: making this a power of 2 (say 2^15) would greatly accelerate fee calcs */
#define ZATTERA_MAX_AUTHORITY_MEMBERSHIP        40
#define ZATTERA_MAX_ASSET_WHITELIST_AUTHORITIES 10
#define ZATTERA_MAX_URL_LENGTH                  127

#define ZATTERA_IRREVERSIBLE_THRESHOLD          (75 * ZATTERA_1_PERCENT)

#define ZATTERA_VIRTUAL_SCHEDULE_LAP_LENGTH  ( fc::uint128(uint64_t(-1)) )
#define ZATTERA_VIRTUAL_SCHEDULE_LAP_LENGTH2 ( fc::uint128::max_value() )

#define ZATTERA_INITIAL_VOTE_POWER_RATE (40)
#define ZATTERA_REDUCED_VOTE_POWER_RATE (10)

#define ZATTERA_MAX_LIMIT_ORDER_EXPIRATION     (60*60*24*28) // 28 days
#define ZATTERA_DELEGATION_RETURN_PERIOD       (ZATTERA_VOTE_REGENERATION_SECONDS * 2)

/**
 *  Reserved Account IDs with special meaning
 */
///@{
/// Represents the canonical account with NO authority (nobody can access funds in null account)
#define ZATTERA_NULL_ACCOUNT                    "null"
/// Represents the canonical account with WILDCARD authority (anybody can access funds in temp account)
#define ZATTERA_TEMP_ACCOUNT                    "temp"
/// Represents the canonical account for specifying you will vote for directly (as opposed to a proxy)
#define ZATTERA_PROXY_TO_SELF_ACCOUNT           ""
/// Represents the canonical root post parent account
#define ZATTERA_ROOT_POST_PARENT                (account_name_type())
///@}
