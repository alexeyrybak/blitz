<?

$body = <<<BODY
<!-- BEGIN test -->
<!-- END -->
BODY;

$T = new Blitz();
$T->load($body);

echo $T->parse();

?>
