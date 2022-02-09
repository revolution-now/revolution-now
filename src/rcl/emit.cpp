/****************************************************************
**emit.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-02-08.
*
* Description: Rcl language emitter.
*
*****************************************************************/
#include "emit.hpp"

// base
#include "base/to-str.hpp"

using namespace std;

namespace rcl {

namespace {

struct emitter {
  emitter( EmitOptions const& options ) : opts_( options ) {}

  void do_indent( int level, string& out ) {
    CHECK_GE( level, 0 );
    for( int i = 0; i < level; ++i ) out += "  ";
  }

  void emit( null_t const&, string& out, int ) { out += "null"; }

  void emit( double const& o, string& out, int ) {
    to_str( o, out, base::ADL );
  }

  void emit( int const& o, string& out, int ) {
    to_str( o, out, base::ADL );
  }

  void emit( bool const& o, string& out, int ) {
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

    if( !nested_val.holds<unique_ptr<table>>() ) out += ": ";
    if( nested_val.holds<unique_ptr<table>>() &&
        nested_val.get<unique_ptr<table>>()->size() != 1 )
      out += ' ';

    std::visit( [&]( auto const& o ) { emit( o, out, indent ); },
                nested_val );
  }

  // We need flatten_immediate so that we can tell this function
  // to not flatten at this level only, but to revert to the
  // value set in opts_ when it recurses. This is because, when
  // flattening is enabled, we need to disable it when a table is
  // a list element, but then only at the top level.
  void emit_table( table const& o, string& out, int indent,
                   bool flatten_immediate ) {
    bool is_top_level = ( indent == 0 );
    if( o.size() == 0 ) {
      if( !is_top_level ) out += "{}";
      return;
    }
    if( flatten_immediate && o.size() == 1 ) {
      emit_table_flatten( o, out, indent );
      return;
    }
    if( !is_top_level ) out += "{\n";

    size_t n = o.size();
    for( auto& [k, v] : o ) {
      string_view assign = ": ";
      if( v.holds<unique_ptr<table>>() ) {
        assign = " ";
        // Here use the one from the opts, as opposed to
        // flatten_immediate, because we are looking at a key in-
        // side this table, so it is not really the immediate
        // context.
        if( opts_.flatten_keys &&
            v.get<unique_ptr<table>>()->size() == 1 )
          assign = "";
      }
      string k_str = escape_and_quote_table_key( k );
      do_indent( indent, out );
      out += k_str;
      out += assign;
      std::visit(
          [&]( auto const& o ) { emit( o, out, indent + 1 ); },
          v );
      out += '\n';
      if( is_top_level && n-- > 1 ) out += '\n';
    }

    if( !is_top_level ) {
      do_indent( indent - 1, out );
      out += '}';
    }
  }

  void emit( unique_ptr<table> const& o, string& out,
             int indent ) {
    emit_table( *o, out, indent, opts_.flatten_keys );
  }

  struct list_visitor {
    void operator()( unique_ptr<table> const& o ) const {
      parent.emit_table( *o, out, indent,
                         /*flatten_immediate=*/false );
    }
    void operator()( auto const& o ) const {
      parent.emit( o, out, indent );
    }
    emitter& parent;
    string&  out;
    int      indent;
  };

  void emit( unique_ptr<list> const& o, string& out,
             int indent ) {
    if( indent > 0 ) out += "[\n";

    for( value const& v : *o ) {
      do_indent( indent, out );
      std::visit( list_visitor{ *this, out, indent + 1 }, v );
      out += ",\n";
    }

    do_indent( indent - 1, out );
    out += ']';
  }

  EmitOptions opts_;
};

} // namespace

string emit( doc const& document, EmitOptions const& options ) {
  string res;
  emitter{ options }.emit_table(
      document.top_tbl(), res,
      /*indent=*/0,
      /*flatten_immediate=*/options.flatten_keys );
  return res;
}

} // namespace rcl
