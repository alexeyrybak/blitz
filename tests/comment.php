<?

/*
$T = new Blitz();
$T->load('<!-- end of Hotlog counter -->');
echo $T->parse();
unset($T);
*/

$T = new Blitz();
$T->load("<!-- some>
{{ a }}
</some -->");

$T->dumpStruct();

echo $T->parse(array('a'=>'a_val'));

?>
