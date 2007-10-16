--TEST--
escape output wrapper
--FILE--
<?php
include('common.inc');

$T = new Blitz();
$T->load('{{ escape($a); }}
{{ escape($b) }}
{{ escape($c,"ENT_COMPAT") }}
{{ escape($c,"ENT_NOQUOTES") }}
');

$T->set(
    array(
        'a' => "this is 'a' \"test\"",
        'b' => '<a href="null">',
        'c' => '<>@#$%^&*()_=\'"'
    )
);

echo $T->parse();

?>
--EXPECT--
this is &#039;a&#039; &quot;test&quot;
&lt;a href=&quot;null&quot;&gt;
&lt;&gt;@#$%^&amp;*()_='&quot;
&lt;&gt;@#$%^&amp;*()_='"
