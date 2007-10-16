--TEST--
{{ $hash.sub.key }} syntax
--FILE--
<?php
include('common.inc');

class DummyObjWithSubProperty {
   var $sub;
   function DummyObjWithSubProperty($val) {
       $this->sub = array('key' => $val);
   }
}

class DummyObjWithHtmlProperty {
   var $html = '<br>';
} 

class View extends Blitz {
    function method($p) {
        return $p;
    }
}

$T = new View();
$T->load(
"{{ BEGIN test}}{{ \$hash.sub.key }} {{ END}}
method: {{ method(\$arr.hello) }}
if: {{ if(\$arr.bool,'true','false') }}
if: {{ if(\$arr.boo,'true','false') }}
excape: {{ escape(\$obj.html) }}
global: {{ \$config.param }}
");

$data['test'] = array();

// mixing arrays and objects for the path "hash.sub.key"
foreach (array('Dude', 'Sobchak', 'Donny') as $i_name) {
    $data['test'][] = array(
        'hash' => new DummyObjWithSubProperty($i_name)
    );
}

$data['arr'] = array(
    'hello' => 'Hello, world!',
    'bool' => 1
);

$data['obj'] = new DummyObjWithHtmlProperty();

$config = array(
    'config' => array(
        'param' => 'value'
    )
);
$T->setGlobals($config);

echo $T->parse($data);

?>
--EXPECT--
Dude Sobchak Donny 
method: Hello, world!
if: true
if: false
excape: &lt;br&gt;
global: value
