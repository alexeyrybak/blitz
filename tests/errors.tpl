{{ $hi }}                   /* OK */
    {{ ''s  }}              /* ERROR */
  {{ $ss }}                 /* OK */
{{ .lskl }}                 /* ERROR */
  {{ -+++ }}                /* ERROR */ 
    {{ _() }}               /* OK */
}}                          /* ERROR */
{{                          /* ERROR */
      {{ x(); }}            /* OK */
    {{ # a }}               /* ERROR */
  {{ END }}                 /* ERROR */
{{ if(1,2,3,4) }}           /* ERROR */
  {{ include( ) }}          /* ERROR */
  {{ include('a','b') }}    /* ERROR */
  {{ include('a','b','c') }}/* ERROR */
  {{ include('a','b'=) }}   /* ERROR */
  {{ include('a','b'=1) }}  /* ERROR */
{{  BEGIN hello }}          /* OK */
    {{ -hello }}            /* ERROR */
{{ test(-1,TRUE,"hi"); }}   /* OK */
{{ END }}                   /* OK */
