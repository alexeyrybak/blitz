--TEST--
object support for begin tag
--FILE--
<?php

include('common.inc');
ini_set('blitz.remove_spaces_around_context_tags', 1);

$body = <<<BODY
{{ BEGIN user }}
  Hi, my name is {{ name }}!
{{ END }}
BODY;

class User {
  var $name;

  function __construct($name) {
    $this->name = $name;
  }
}

$T = new Blitz();
$T->load($body);
$T->set(array('user' => new User('Maurus')));
$T->display();

?>
--EXPECT--
Hi, my name is Maurus!