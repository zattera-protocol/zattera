#pragma once

namespace fc {
class uint128;
class variant;
} // fc

namespace zattera { namespace protocol {
template< typename Storage = fc::uint128 >
class fixed_string_impl;

class asset_symbol_type;
class legacy_zattera_asset_symbol_type;
struct legacy_zattera_asset;
} } // zattera::protocol

namespace fc { namespace raw {

template<typename Stream>
inline void pack( Stream& s, const uint128& u );
template<typename Stream>
inline void unpack( Stream& s, uint128& u );

template< typename Stream, typename Storage >
inline void pack( Stream& s, const zattera::protocol::fixed_string_impl< Storage >& u );
template< typename Stream, typename Storage >
inline void unpack( Stream& s, zattera::protocol::fixed_string_impl< Storage >& u );

template< typename Stream >
inline void pack( Stream& s, const zattera::protocol::asset_symbol_type& sym );
template< typename Stream >
inline void unpack( Stream& s, zattera::protocol::asset_symbol_type& sym );

template< typename Stream >
inline void pack( Stream& s, const zattera::protocol::legacy_zattera_asset_symbol_type& sym );
template< typename Stream >
inline void unpack( Stream& s, zattera::protocol::legacy_zattera_asset_symbol_type& sym );

} // raw

template< typename Storage >
inline void to_variant( const zattera::protocol::fixed_string_impl< Storage >& s, fc::variant& v );
template< typename Storage >
inline void from_variant( const variant& v, zattera::protocol::fixed_string_impl< Storage >& s );

inline void to_variant( const zattera::protocol::asset_symbol_type& sym, fc::variant& v );

inline void from_variant( const fc::variant& v, zattera::protocol::legacy_zattera_asset& leg );
inline void to_variant( const zattera::protocol::legacy_zattera_asset& leg, fc::variant& v );

} // fc
