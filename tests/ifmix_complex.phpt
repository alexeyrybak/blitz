--TEST--
conditional mix
--FILE--
<?php
include('common.inc');

$T = new Blitz();
$body = <<<BODY
== boolean ==
{{ if(a && b,'true','false'); }}
{{ IF(a || b,'true','false'); }}
{{ unless(c || d,'true','false'); }}
{{ UNLESS(c && d,'true','false'); }}
{{ if(a || e,'true','false'); }}
{{ IF(a && e,'true','false'); }}
{{ unless(d && e,'true','false'); }}
{{ UNLESS(d || e,'true','false'); }}
{{ if(a && b,'true'); }}-
{{ IF(a || b,'true'); }}-
{{ unless(c || d,'true'); }}-
{{ UNLESS(c && d,'true'); }}-
{{ if(a || e,'true'); }}-
{{ IF(a && e,'true'); }}-
{{ unless(d && e,'true'); }}-
{{ UNLESS(d || e,'true'); }}-
== numerical ==
{{ if(n == 1 || a,'true','false'); }}
{{ if(n >= 2 || a,'true','false'); }}
{{ unless(n == 1 || a,'true','false'); }}
{{ unless(n >= 2 || a,'true','false'); }}
{{ if(n == 1 || b,'true','false'); }}
{{ if(n >= 2 || b,'true','false'); }}
{{ unless(n == 1 || b,'true','false'); }}
{{ unless(n >= 2 || b,'true','false'); }}
{{ if(n == 1 || a,'true'); }}-
{{ if(n >= 2 || a,'true'); }}-
{{ unless(n == 1 || a,'true'); }}-
{{ unless(n >= 2 || a,'true'); }}-
{{ if(n == 1 || b,'true'); }}-
{{ if(n >= 2 || b,'true'); }}-
{{ unless(n == 1 || b,'true'); }}-
{{ unless(n >= 2 || b,'true'); }}-
== string ==
{{ if(s == 's','true','false'); }}
{{ if(s != "s",'true','false'); }}
{{ if(s,'true','false'); }}
{{ unless(s == 's','true','false'); }}
{{ unless(s != "s",'true','false'); }}
{{ unless(s,'true','false'); }}
{{ if(s == 's','true'); }}-
{{ if(s != "s",'true'); }}-
{{ if(s,'true'); }}-
{{ unless(s == 's','true'); }}-
{{ unless(s != "s",'true'); }}-
{{ unless(s,'true'); }}-
== variables ==
{{ if(\$a || \$b,'true','false'); }}
{{ if(\$c || \$e,'true','false'); }}
{{ if(\$a || (\$a && \$a),'true','false'); }}
{{ if((\$a || \$a) && \$a,'true','false'); }}
{{ if(\$a || \$a && \$a,'true','false'); }}
{{ if(\$b || (\$a && \$b),'true','false'); }}
{{ if((\$b || \$a) && \$b,'true','false'); }}
{{ if(\$b || \$a && \$b,'true','false'); }}
== path variables ==
{{ if(\$arr.name == "dude",'true','false'); }}
{{ if(\$arr.name != "dude",'true','false'); }}
{{ unless(\$arr.name == "dude",'true','false'); }}
{{ unless(\$arr.name != "dude",'true','false'); }}
{{ if(\$arr.name == "dude",'true'); }}-
{{ if(\$arr.name != "dude",'true'); }}-
{{ unless(\$arr.name == "dude",'true'); }}-
{{ unless(\$arr.name != "dude",'true'); }}-
{{ if(\$arr.name,'true','false'); }}
{{ unless(\$arr.name,'true','false'); }}
{{ if(\$arr.name,'true'); }}-
{{ unless(\$arr.name,'true'); }}-

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
    'n' => 2,
    's' => 's',
    'arr' => array(
        'name' => 'dude'
    )
);

$T->display($params);

?>
--EXPECT--
== boolean ==
false
true
false
false
true
false
true
false
-
true-
-
-
true-
-
true-
-
== numerical ==
true
true
false
false
false
true
true
false
true-
true-
-
-
-
true-
true-
-
== string ==
true
false
true
false
true
false
true-
-
true-
-
true-
-
== variables ==
true
true
true
true
true
false
false
false
== path variables ==
true
false
false
true
true-
-
-
true-
true
false
true-
-
