--TEST--
scope test with magic variables (_top and _parent)
--FILE--
<?php
include('common.inc');
ini_set('blitz.remove_spaces_around_context_tags', 1);

$T = new Blitz();

$body = <<<BODY
{{ BEGIN users }}
Name: {{ name }} {{ IF name == _top.currentUserName }}(Current user){{ END }}
Friends:
  {{ BEGIN friends }}
  Name: {{ name }} {{ IF name == _top.currentUserName }}(Current user){{ END }}
  Friends:
    {{ BEGIN friends }}
      Name: {{ name }} {{ IF name == _parent.name }}(I'm friends with myself!){{ END }}{{ IF name == _parent._parent.name }}(Mutual!){{ END }}{{ IF name == _top.currentUserName }}(Current user){{ END }}
    {{ END friends }}
  {{ END friends }}
{{ END users }}
===================================================================

BODY;

$T->load($body);

$item = new stdClass();
$item->upper = 'OK';
$item->inner = array(
    'value' => true
);

$vincent = array("name" => "Vincent", "age" => 32);
$maurus = array("name" => "Maurus", "age" => 23);
$thibaut = array("name" => "Thibaut", "age" => 16);
$nicolas = array("name" => "Nicolas", "age" => 31);

$vincent["friends"] = array(&$maurus, &$thibaut, &$vincent);
$maurus["friends"] = array(&$thibaut, &$vincent, &$nicolas);
$thibaut["friends"] = array(&$vincent);

$data = array(
    'users' => array(
        $vincent,
	$maurus,
	$thibaut,
	$nicolas
    ),
    'currentUserName' => 'Thibaut'
);

ini_set('blitz.enable_magic_scope', 0);
$T->display($data);
ini_set('blitz.enable_magic_scope', 1);
$T->display($data);

?>
--EXPECT--
Name: Vincent 
Friends:
  Name: Maurus 
  Friends:
      Name: Thibaut 
      Name: Vincent 
      Name: Nicolas 
  Name: Thibaut 
  Friends:
      Name: Vincent 
  Name: Vincent 
  Friends:
      Name: Maurus 
      Name: Thibaut 
      Name: Vincent 
Name: Maurus 
Friends:
  Name: Thibaut 
  Friends:
      Name: Vincent 
  Name: Vincent 
  Friends:
      Name: Maurus 
      Name: Thibaut 
      Name: Vincent 
  Name: Nicolas 
  Friends:
Name: Thibaut 
Friends:
  Name: Vincent 
  Friends:
      Name: Maurus 
      Name: Thibaut 
      Name: Vincent 
Name: Nicolas 
Friends:
===================================================================
Name: Vincent 
Friends:
  Name: Maurus 
  Friends:
      Name: Thibaut (Current user)
      Name: Vincent (Mutual!)
      Name: Nicolas 
  Name: Thibaut (Current user)
  Friends:
      Name: Vincent (Mutual!)
  Name: Vincent 
  Friends:
      Name: Maurus 
      Name: Thibaut (Current user)
      Name: Vincent (I'm friends with myself!)(Mutual!)
Name: Maurus 
Friends:
  Name: Thibaut (Current user)
  Friends:
      Name: Vincent 
  Name: Vincent 
  Friends:
      Name: Maurus (Mutual!)
      Name: Thibaut (Current user)
      Name: Vincent (I'm friends with myself!)
  Name: Nicolas 
  Friends:
Name: Thibaut (Current user)
Friends:
  Name: Vincent 
  Friends:
      Name: Maurus 
      Name: Thibaut (Mutual!)(Current user)
      Name: Vincent (I'm friends with myself!)
Name: Nicolas 
Friends:
===================================================================
