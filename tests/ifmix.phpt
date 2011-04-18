--TEST--
conditional mix
--FILE--
<?php
include('common.inc');

$T = new Blitz();
$body = <<<BODY
== boolean ==
{{ if(true,'true','false'); }}
{{ IF(true,'true','false'); }}
{{ unless(true,'true','false'); }}
{{ UNLESS(true,'true','false'); }}
{{ if(false,'true','false'); }}
{{ IF(false,'true','false'); }}
{{ unless(false,'true','false'); }}
{{ UNLESS(false,'true','false'); }}
== numerical ==
{{ if(1,'true','false'); }}
{{ if(0,'true','false'); }}
{{ unless(1,'true','false'); }}
{{ unless(0,'true','false'); }}
== string ==
{{ if('','true','false'); }}
{{ if('0','true','false'); }}
{{ if('string','true','false'); }}
== variables ==
{{ if(\$a,'true','false'); }}
{{ if(\$b,'true','false'); }}
{{ if(\$c,'true','false'); }}
{{ if(\$d,'true','false'); }}
{{ if(\$e,'true','false'); }}
{{ if(\$f,'true','false'); }}
{{ if(\$g,'true','false'); }}
{{ if(\$h,'true','false'); }}
== path variables ==
{{ if(\$arr.name,'true','false'); }}
BODY;
$T->load($body);
$params = array(
    'a' => true,
    'b' => false,
    'c' => 1,
    'd' => 0.1,
    'e' => 0,
    'f' => '0',
    'g' => '1',
    'h' => 's',
    'arr' => array(
        'name' => 'dude'
    )
);

$T->display($params);

?>
--EXPECT--
== boolean ==
true
true
false
false
false
false
true
true
== numerical ==
true
false
false
true
== string ==
false
false
true
== variables ==
true
false
true
true
false
false
true
true
== path variables ==
true
