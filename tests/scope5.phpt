--TEST--
parent predefined and custom variables 
--FILE--
<?php
include "common.inc";

$T = new Blitz();
$tmp = "
{{BEGIN cycle1}}
	{{test}}
	iteration: [{{_num}}]
	{{BEGIN children}}
		parent iteration: [{{_parent._num}}]
		my iteration: [{{_num}}]
		parent var: [{{_parent.test}}]
	{{END}}
{{END}}";

$T->load($tmp);

$T->block('/cycle1', array("test"=>"value1"));
$T->block('/cycle1/children', array());
$T->block('/cycle1', array("test"=>"value2"));
$T->block('/cycle1/children', array());

echo $T->parse();

echo "END\n";
?>
--EXPECTF--
value1
	iteration: [1]
	
		parent iteration: [1]
		my iteration: [1]
		parent var: [value1]
	

	value2
	iteration: [2]
	
		parent iteration: [2]
		my iteration: [1]
		parent var: [value2]
	
END
