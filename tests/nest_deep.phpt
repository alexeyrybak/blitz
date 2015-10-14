--TEST--
testing deep nesting
--FILE--
<?php
include('common.inc');

ini_set('blitz.remove_spaces_around_context_tags', 1);
ini_set('blitz.warn_context_duplicates', 0);

$T = new Blitz();
 
$body = <<<BODY
name: {{struct.name}}
age: {{struct.age}}

friends:
{{ BEGIN struct }}
{{ BEGIN friends }}
  = {{ name }}
  {{ BEGIN friends }}
    - {{ name }}
    {{ IF friends }}
      - Friend of {{ name }}
    {{ END friends }}
  {{ END friends }}
{{ END friends }}
{{ END struct }}

friends:
{{ BEGIN struct }}
{{ BEGIN friends }}
  = {{ name }}
  {{ BEGIN friends }}
    - {{ name }}
    {{ IF friends }}
      - Friend of {{ name }}:
      {{ BEGIN friends }}
        - {{ name }}
      {{ END friends }}
    {{ END friends }}
  {{ END friends }}
{{ END friends }}
{{ END struct }}
===================================================================

BODY;

$T->load($body);

$vincent = array("name" => "Vincent", "age" => 32);
$maurus = array("name" => "Maurus", "age" => 23);
$thibaut = array("name" => "Thibaut", "age" => 16);
$nicolas = array("name" => "Nicolas", "age" => 31);

$vincent["friends"] = array(&$maurus, &$thibaut);
$maurus["friends"] = array(&$thibaut, &$vincent, &$nicolas);
$thibaut["friends"] = array(&$vincent);

$data = array(
    'struct' => $vincent
);

$T->display($data);

?>
--EXPECT--
name: Vincent
age: 32

friends:
  = Maurus
    - Thibaut
      - Friend of Thibaut
    - Vincent
      - Friend of Vincent
    - Nicolas
  = Thibaut
    - Vincent
      - Friend of Vincent

friends:
  = Maurus
    - Thibaut
      - Friend of Thibaut:
        - Vincent
    - Vincent
      - Friend of Vincent:
        - Maurus
        - Thibaut
    - Nicolas
  = Thibaut
    - Vincent
      - Friend of Vincent:
        - Maurus
        - Thibaut
===================================================================
