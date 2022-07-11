#include "fd_tango.h"
#include "mcache/fd_mcache_private.h"

#if FD_HAS_HOSTED && FD_HAS_X86

#include <stdio.h>

int
main( int     argc,
      char ** argv ) {
  fd_boot( &argc, &argv );
# define SHIFT(n) argv+=(n),argc-=(n)

  if( FD_UNLIKELY( argc<1 ) ) FD_LOG_ERR(( "no arguments" ));
  char const * bin = argv[0];
  SHIFT(1);

  /* FIXME: CACHE ATTACHEMENTS? */

  int cnt = 0;
  while( argc ) {
    char const * cmd = argv[0];
    SHIFT(1);

    if( !strcmp( cmd, "help" ) ) {

      FD_LOG_NOTICE(( "\n\t"
        "Usage: %s [cmd] [cmd args] [cmd] [cmd args] ...\n\t"
        "Commands are:\n\t"
        "\n\t"
        "\thelp\n\t"
        "\t- Prints this message\n\t"
        "\n\t"
        "\tnew-mcache wksp depth init\n\t"
        "\t- Creates a frag meta data cache in wksp with the given depth\n\t"
        "\t  and initial sequence number.  Prints the wksp gaddr of the\n\t"
        "\t  mcache to stdout.\n\t"
        "\n\t"
        "\tdelete-mcache gaddr\n\t"
        "\t- Destroys the mcache at gaddr.\n\t"
        "\n\t"
        "\tquery-mcache gaddr\n\t"
        "\t- Queries the mcache at gaddr.\n\t"
        "\n\t", bin ));
      FD_LOG_NOTICE(( "%i: %s: success", cnt, cmd ));

    } else if( !strcmp( cmd, "new-mcache" ) ) {

      if( FD_UNLIKELY( argc<3 ) ) FD_LOG_ERR(( "%i: %s: too few arguments\n\tDo %s help for help", cnt, cmd, bin ));

      char const * _wksp =                   argv[0];
      ulong        depth = fd_cstr_to_ulong( argv[1] );
      ulong        init  = fd_cstr_to_ulong( argv[2] );

      ulong align     = fd_mcache_align();
      ulong footprint = fd_mcache_footprint( depth );
      if( FD_UNLIKELY( !footprint ) ) FD_LOG_ERR(( "%i: %s: depth (%lu) must a power-of-2 and at least %lu\n\tDo %s help for help",
                                                   cnt, cmd, depth, FD_MCACHE_BLOCK, bin ));

      fd_wksp_t * wksp = fd_wksp_attach( _wksp );
      if( FD_UNLIKELY( !wksp ) ) {
        FD_LOG_ERR(( "%i: %s: fd_wksp_attach( \"%s\" ) failed\n\tDo %s help for help", cnt, cmd, _wksp, bin ));
      }

      ulong gaddr = fd_wksp_alloc( wksp, align, footprint );
      if( FD_UNLIKELY( !gaddr ) ) {
        fd_wksp_detach( wksp );
        FD_LOG_ERR(( "%i: %s: fd_wksp_alloc( \"%s\", %lu, %lu ) failed\n\tDo %s help for help",
                     cnt, cmd, _wksp, align, footprint, bin ));
      }

      void * shmem = fd_wksp_laddr( wksp, gaddr );
      if( FD_UNLIKELY( !shmem ) ) {
        fd_wksp_free( wksp, gaddr );
        fd_wksp_detach( wksp );
        FD_LOG_ERR(( "%i: %s: fd_wksp_laddr( \"%s\", %lu ) failed\n\tDo %s help for help", cnt, cmd, _wksp, gaddr, bin ));
      }

      void * shmcache = fd_mcache_new( shmem, depth, init );
      if( FD_UNLIKELY( !shmcache ) ) {
        fd_wksp_free( wksp, gaddr );
        fd_wksp_detach( wksp );
        FD_LOG_ERR(( "%i: %s: fd_mcache_new( %s:%lu, %lu, %lu ) failed\n\tDo %s help for help",
                     cnt, cmd, _wksp, gaddr, depth, init, bin ));
      }

      char buf[ FD_WKSP_CSTR_MAX ];
      printf( "%s\n", fd_wksp_cstr( wksp, gaddr, buf ) );

      fd_wksp_detach( wksp );

      FD_LOG_NOTICE(( "%i: %s %s %lu %lu: success", cnt, cmd, _wksp, depth, init ));
      SHIFT( 3 );

    } else if( !strcmp( cmd, "delete-mcache" ) ) {

      if( FD_UNLIKELY( argc<1 ) ) FD_LOG_ERR(( "%i: %s: too few arguments\n\tDo %s help for help", cnt, cmd, bin ));

      char const * _shmcache = argv[0];

      void * shmcache = fd_wksp_map( _shmcache );
      if( FD_UNLIKELY( !shmcache ) )
        FD_LOG_ERR(( "%i: %s: fd_wksp_map( \"%s\" ) failed\n\tDo %s help for help", cnt, cmd, _shmcache, bin ));
      if( FD_UNLIKELY( !fd_mcache_delete( shmcache ) ) )
        FD_LOG_ERR(( "%i: %s: fd_mcache_delete( \"%s\" ) failed\n\tDo %s help for help", cnt, cmd, _shmcache, bin ));
      fd_wksp_unmap( shmcache );

      fd_wksp_cstr_free( _shmcache );
      
      FD_LOG_NOTICE(( "%i: %s %s: success", cnt, cmd, _shmcache ));
      SHIFT( 1 );

    } else if( !strcmp( cmd, "query-mcache" ) ) {

      if( FD_UNLIKELY( argc<1 ) ) FD_LOG_ERR(( "%i: %s: too few arguments\n\tDo %s help for help", cnt, cmd, bin ));

      char const * _shmcache = argv[0];

      void * shmcache = fd_wksp_map( _shmcache );
      if( FD_UNLIKELY( !shmcache ) )
        FD_LOG_ERR(( "%i: %s: fd_wksp_map( \"%s\" ) failed\n\tDo %s help for help", cnt, cmd, _shmcache, bin ));

      fd_frag_meta_t * mcache = fd_mcache_join( shmcache );
      if( FD_UNLIKELY( !mcache ) )
        FD_LOG_ERR(( "%i: %s: fd_mcache_join( \"%s\" ) failed\n\tDo %s help for help", cnt, cmd, _shmcache, bin ));

      fd_mcache_private_hdr_t * hdr = fd_mcache_private_hdr( mcache );
      printf( "mcache %s\n", _shmcache );
      printf( "\tdepth   %lu\n", hdr->depth );
      printf( "\tseq0    %lu\n", hdr->seq0  );

      ulong seq_cnt;
      for( seq_cnt=FD_MCACHE_SEQ_CNT; seq_cnt; seq_cnt-- ) if( hdr->seq[seq_cnt-1UL] ) break;
      printf( "\tseq[ 0] %lu\n", *hdr->seq );
      for( ulong idx=1UL; idx<seq_cnt; idx++ ) printf( "\tseq[%2lu] %lu\n", idx, hdr->seq[idx] );
      if( seq_cnt<FD_MCACHE_SEQ_CNT ) printf( "\t        ... snip (all remaining are zero) ...\n" );

      ulong app_sz;
      for( app_sz=FD_MCACHE_APP_FOOTPRINT; app_sz; app_sz-- ) if( hdr->app[app_sz-1UL] ) break;
      ulong   off = 0UL;
      uchar * a   = hdr->app;
      printf( "\tapp     %04lx: %02x %02x %02x %02x %02x %02x %02x %02x  %02x %02x %02x %02x %02x %02x %02x %02x\n", off,
              (uint)a[ 0], (uint)a[ 1], (uint)a[ 2], (uint)a[ 3], (uint)a[ 4], (uint)a[ 5], (uint)a[ 6], (uint)a[ 7],
              (uint)a[ 8], (uint)a[ 9], (uint)a[10], (uint)a[11], (uint)a[12], (uint)a[13], (uint)a[14], (uint)a[15] );
      for( off+=16UL, a+=16UL; off<app_sz; off+=16UL, a+=16UL )
        printf( "\t        %04lx: %02x %02x %02x %02x %02x %02x %02x %02x  %02x %02x %02x %02x %02x %02x %02x %02x\n", off,
                (uint)a[ 0], (uint)a[ 1], (uint)a[ 2], (uint)a[ 3], (uint)a[ 4], (uint)a[ 5], (uint)a[ 6], (uint)a[ 7],
                (uint)a[ 8], (uint)a[ 9], (uint)a[10], (uint)a[11], (uint)a[12], (uint)a[13], (uint)a[14], (uint)a[15] );
      if( off<FD_MCACHE_APP_FOOTPRINT ) printf( "\t        ... snip (all remaining are zero) ...\n" );

      fd_wksp_unmap( fd_mcache_leave( mcache ) );
      
      FD_LOG_NOTICE(( "%i: %s %s: success", cnt, cmd, _shmcache ));
      SHIFT( 1 );

    } else {

      FD_LOG_ERR(( "%i: %s: unknown command\n\t"
                   "Do %s help for help", cnt, cmd, bin ));

    }
    cnt++;
  }

  FD_LOG_NOTICE(( "processed %i commands", cnt ));

# undef SHIFT
  fd_halt();
  return 0;
}

#else

int
main( int     argc,
      char ** argv ) {
  fd_boot( &argc, &argv );
  if( FD_UNLIKELY( argc<1 ) ) FD_LOG_ERR(( "No arguments" ));
  if( FD_UNLIKELY( argc>1 ) ) FD_LOG_ERR(( "fd_tango_ctl not supported on this platform" ));
  FD_LOG_NOTICE(( "processed 0 commands" ));
  fd_halt();
  return 0;
}

#endif
