/****************************************************************
**emit.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-02-08.
*
* Description: Rcl language emitters.
*
*****************************************************************/
#include "emit.hpp"

// base
#include "base/to-str.hpp"

using namespace std;

using ::cdr::list;
using ::cdr::null_t;
using ::cdr::table;
using ::cdr::value;

namespace rcl {

namespace {

/****************************************************************
** Helpers.
*****************************************************************/
string json_quote( string const& s ) {
  string res;
  for( char c : s ) {
    if( c == '"' ) res += '\\';
    if( c == '\\' ) res += '\\';
    res += c;
  }
  return '"' + res + '"';
}

/****************************************************************
** Standard rcl emitter.
*****************************************************************/
struct standard_emitter {
  standard_emitter( EmitOptions const& options )
    : opts_( options ) {}

  void do_indent( int level, string& out ) {
    CHECK_GE( level, 0 );
    for( int i = 0; i < level; ++i ) out += "  ";
  }

  void emit( null_t, string& out, int ) { out += "null"; }

  void emit( double o, string& out, int ) {
    to_str( o, out, base::ADL );
  }

  void emit( cdr::integer_type o, string& out, int ) {
    to_str( o, out, base::ADL );
  }

  void emit( bool o, string& out, int ) {
    to_str( o, out, base::ADL );
  }

  void emit( string const& o, string& out, int ) {
    out += escape_and_quote_string_val( o );
  }

  void emit_table_flatten( table const& o, string& out,
                           int indent ) {
    CHECK( opts_.flatten_keys );
    CHECK( o.size() == 1 );
    string const& nested_key = o.begin()->first;
    value const&  nested_val = o.begin()->second;

    out += '.';
    out += escape_and_quote_table_key( nested_key );

    if( !nested_val.holds<table>() ) out += ": ";
    if( nested_val.holds<table>() &&
        nested_val.get<table>().size() != 1 )
      out += ' ';

    base::visit(
        [&]( auto const& o ) { emit( o, out, indent ); },
        nested_val.as_base() );
  }

  void emit_table( table const& o, string& out, int indent ) {
    bool is_top_level = ( indent == 0 );
    if( o.size() == 0 ) {
      if( !is_top_level ) out += "{}";
      return;
    }
    if( opts_.flatten_keys && o.size() == 1 ) {
      emit_table_flatten( o, out, indent );
      return;
    }
    if( !is_top_level ) out += "{\n";

    size_t n = o.size();
    for( auto& [k, v] : o ) {
      string_view assign = ": ";
      if( v.holds<table>() ) {
        assign = " ";
        if( opts_.flatten_keys && v.get<table>().size() == 1 )
          assign = "";
      }
      string k_str = escape_and_quote_table_key( k );
      do_indent( indent, out );
      out += k_str;
      out += assign;
      base::visit(
          [&]( auto const& o ) { emit( o, out, indent + 1 ); },
          v.as_base() );
      out += '\n';
      if( is_top_level && n-- > 1 ) out += '\n';
    }

    if( !is_top_level ) {
      do_indent( indent - 1, out );
      out += '}';
    }
  }

  void emit( table const& o, string& out, int indent ) {
    emit_table( o, out, indent );
  }

  struct list_visitor {
    void operator()( auto const& o ) const {
      parent.emit( o, out, indent );
    }
    standard_emitter& parent;
    string&           out;
    int               indent;
  };

  void emit( list const& o, string& out, int indent ) {
    if( indent > 0 ) {
      out += '[';
      if( o.empty() ) {
        out += "]";
        return;
      }
      out += '\n';
    }

    for( value const& v : o ) {
      do_indent( indent, out );
      base::visit( list_visitor{ *this, out, indent + 1 },
                   v.as_base() );
      out += ",\n";
    }

    do_indent( indent - 1, out );
    out += ']';
  }

  EmitOptions opts_;
};

/****************************************************************
** JSON emitter.
*****************************************************************/
// This one is basically just a stripped down version of the
// above, with some extra JSON-confirming things like quotes and
// commas, along with the ability to order keys.
//
// Note that this emitter must support both key-ordered tables
// and non-key-ordered tables, since we may receive a cdr object
// that is a mix of both.
struct json_emitter {
  json_emitter( JsonEmitOptions options )
    : options_( std::move( options ) ) {}

  void do_indent( int level, string& out ) {
    CHECK_GE( level, 0 );
    for( int i = 0; i < level; ++i ) out += "  ";
  }

  void emit( null_t, string& out, int ) { out += "null"; }

  void emit( double o, string& out, int ) {
    to_str( o, out, base::ADL );
  }

  void emit( cdr::integer_type o, string& out, int ) {
    to_str( o, out, base::ADL );
  }

  void emit( bool o, string& out, int ) {
    to_str( o, out, base::ADL );
  }

  void emit( string const& o, string& out, int ) {
    out += json_quote( o );
  }

  void emit_table_with_key_order( table const& o,
                                  list const&  key_order,
                                  string& out, int indent ) {
    // In this function we need to use key_order's size instead
    // of the table's size because the table contains an extra
    // key order tag that will be used but not emitted.
    if( key_order.size() == 0 ) {
      out += "{}";
      return;
    }
    out += "{\n";

    int idx = 0;
    for( auto& kvalue : key_order ) {
      base::maybe<string const&> k = kvalue.get_if<string>();
      if( !k.has_value() ) continue;
      base::maybe<cdr::value const&> v = o[*k];
      if( !v.has_value() ) continue;
      do_indent( indent, out );
      out += json_quote( *k );
      out += ": ";
      base::visit(
          [&]( auto const& o ) { emit( o, out, indent + 1 ); },
          v->as_base() );
      if( idx++ < int( key_order.size() - 1 ) ) out += ',';
      out += '\n';
    }

    do_indent( indent - 1, out );
    out += '}';
  }

  void emit_table( table const& o, string& out, int indent ) {
    if( o.size() == 0 ) {
      out += "{}";
      return;
    }
    out += "{\n";

    int idx = 0;
    for( auto& [k, v] : o ) {
      do_indent( indent, out );
      out += json_quote( k );
      out += ": ";
      base::visit(
          [&]( auto const& o ) { emit( o, out, indent + 1 ); },
          v.as_base() );
      if( idx++ < int( o.size() - 1 ) ) out += ',';
      out += '\n';
    }

    do_indent( indent - 1, out );
    out += '}';
  }

  void emit( table const& o, string& out, int indent ) {
    if( options_.key_order_tag.has_value() &&
        o.contains( *options_.key_order_tag ) ) {
      base::maybe<cdr::list const&> const key_order =
          o[*options_.key_order_tag].get_if<cdr::list>();
      if( key_order.has_value() )
        emit_table_with_key_order( o, *key_order, out, indent );
    } else {
      emit_table( o, out, indent );
    }
  }

  struct list_visitor {
    void operator()( auto const& o ) const {
      parent.emit( o, out, indent );
    }
    json_emitter& parent;
    string&       out;
    int           indent;
  };

  void emit( list const& o, string& out, int indent ) {
    if( indent > 0 ) {
      out += '[';
      if( o.empty() ) {
        out += "]";
        return;
      }
      out += '\n';
    }

    int idx = 0;
    for( value const& v : o ) {
      do_indent( indent, out );
      base::visit( list_visitor{ *this, out, indent + 1 },
                   v.as_base() );
      if( idx++ < int( o.size() - 1 ) ) out += ',';
      out += '\n';
    }

    do_indent( indent - 1, out );
    out += ']';
  }

  JsonEmitOptions const options_;
};

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
string emit( doc const& document, EmitOptions const& options ) {
  string res;
  standard_emitter{ options }.emit( document.top_tbl(), res,
                                    /*indent=*/0 );
  return res;
}

string emit_json( doc const&      document,
                  JsonEmitOptions options ) {
  string res;
  json_emitter{ options }.emit( document.top_tbl(), res,
                                /*indent=*/1 );
  return res;
}

} // namespace rcl
