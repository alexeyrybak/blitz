--TEST--
IF doesn't change scope 
--FILE--
<?php

$T = new Blitz();

$tmp = "
{{BEGIN item}}
	{{id}} = 1
	{{BEGIN children}}
		{{id}} = 2
		{{ if \$_parent.id == 1 }}
			Success
			Parent id: {{_parent.id}} is 1
		{{END}}
	{{END}}
{{END}}
";
    
$T->load($tmp);
$T->block('/item', array("id"=>1));
$T->block('/item/children', array("id"=>2));
echo $T->parse();

?>
--EXPECTF--
1 = 1
		2 = 2
			Success
			Parent id: 1 is 1
