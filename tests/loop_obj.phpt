--TEST--
looping over objects
--FILE--
<?php

include('common.inc');
ini_set('blitz.remove_spaces_around_context_tags', 1);

$body = <<<BODY
Hi, I'm {{ user.name }}, and my friends are:
{{ BEGIN user.friends }}
  {{ name }}
{{ END }}
BODY;

class User {
  var $name;
  var $friends;

  function __construct($name) {
    $this->name = $name;
  }
}

$u = new User("Nicolas");
$u->friends = array(new User("Maurus"), new User("Thibaut"));

$T = new Blitz();
$T->load($body);
$T->set(array('user' => $u));
var_dump($T->getStruct());
$T->display();

?>
--EXPECT--
array(4) {
  [0]=>
  string(10) "/user.name"
  [1]=>
  string(14) "/user.friends/"
  [2]=>
  string(18) "/user.friends/name"
  [3]=>
  string(17) "/user.friends/END"
}
Hi, I'm Nicolas, and my friends are:
  Maurus
  Thibaut
