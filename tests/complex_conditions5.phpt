--TEST--
expressions with null and strings
--FILE--
<?php
include('common.inc');
ini_set('blitz.remove_spaces_around_context_tags', 1);

$body = <<<BODY
<select name="type">
{{BEGIN types}}
	<option id="{{id}}" {{IF _parent.type == name}}selected="selected"{{END}}>{{name}}</option>
	<option id="second_{{id}}" {{IF _parent.type && _parent.type == name}}selected="selected"{{END}}>{{name}}</option>
{{END}}
</select>

BODY;

$types = array(
	array('id' => 1, 'name' => 'House'),
	array('id' => 2, 'name' => 'Land'),
	array('id' => 3, 'name' => 'Flat'),
	array('id' => 4, 'name' => 'Garage')
);

$T = new Blitz();
$T->load($body);

// run 1, no assigns
$T->display(
	array(
		'types' => $types
	)
);

// run 2, null assign
$T->display(
	array(
		'types' => $types,
		'type' => null
	)
);

// run 3, full assign
$T->display(
	array(
		'types' => $types,
		'type' => 'Land'
	)
);

?>
--EXPECT--
<select name="type">
	<option id="1" >House</option>
	<option id="second_1" >House</option>
	<option id="2" >Land</option>
	<option id="second_2" >Land</option>
	<option id="3" >Flat</option>
	<option id="second_3" >Flat</option>
	<option id="4" >Garage</option>
	<option id="second_4" >Garage</option>
</select>
<select name="type">
	<option id="1" >House</option>
	<option id="second_1" >House</option>
	<option id="2" >Land</option>
	<option id="second_2" >Land</option>
	<option id="3" >Flat</option>
	<option id="second_3" >Flat</option>
	<option id="4" >Garage</option>
	<option id="second_4" >Garage</option>
</select>
<select name="type">
	<option id="1" >House</option>
	<option id="second_1" >House</option>
	<option id="2" selected="selected">Land</option>
	<option id="second_2" selected="selected">Land</option>
	<option id="3" >Flat</option>
	<option id="second_3" >Flat</option>
	<option id="4" >Garage</option>
	<option id="second_4" >Garage</option>
</select>
