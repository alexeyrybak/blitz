--TEST--
unprefixed variables syntax
--FILE--
<?php

class Dummy {
    var $var;

    function Dummy($value) {
        $this->var = $value;
    }
}

include('common.inc');

$T = new Blitz();
$T->load("{{ var }}\n{{ arr.var }}\n{{ escape(arr.var) }}\n{{ obj.var }}\n{{ escape(obj.var) }}\n");
$T->set(
    array(
        'var' => 'scalar value',
        'arr' => array(
            'var' => '"array" key'
        ),
        'obj' => new Dummy('"object" property')
    )
);

echo $T->parse();

?>
--EXPECT--
scalar value
"array" key
&quot;array&quot; key
"object" property
&quot;object&quot; property
