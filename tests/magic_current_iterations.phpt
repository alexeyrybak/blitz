--TEST--
$_ as current iterations
--FILE--
<?php
include('common.inc');

class View extends Blitz {
    function a ($p) {
        static $calls = 0;
        $this->set(array('a_calls' => ++$calls));
        var_dump($p);
        unset($p);
    }

    function b() {
        static $calls = 0;
        $this->set(array('b_calls' => ++$calls));
    }
}

$T = new View();
$T->set(array('variable1' => 'value1', 'variable2' => 'value2'));

$body = <<<BODY
{{ b() }}
{{ a(_) }}
{{ b() }}
{{ a(_) }}
BODY;

$T->load($body);
$T->display();

var_dump($T->getIterations());

?>
--EXPECT--
array(3) {
  ["variable1"]=>
  string(6) "value1"
  ["variable2"]=>
  string(6) "value2"
  ["b_calls"]=>
  int(1)
}
array(4) {
  ["variable1"]=>
  string(6) "value1"
  ["variable2"]=>
  string(6) "value2"
  ["b_calls"]=>
  int(2)
  ["a_calls"]=>
  int(1)
}



array(1) {
  [0]=>
  array(4) {
    ["variable1"]=>
    string(6) "value1"
    ["variable2"]=>
    string(6) "value2"
    ["b_calls"]=>
    int(2)
    ["a_calls"]=>
    int(2)
  }
}
