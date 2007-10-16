<?

$body = '<!-- BEGIN test --> {{ $alpha }} {{ $omega }} <!-- END -->';

$T = new Blitz();
$T->load($body);

var_dump($T->getStruct());

/* this will dump full paths for all the structure "nodes":

array(3) {
  [0]=>
  string(6) "/test/"   // CONTEXTS ARE ENDED WITH '/'
  [1]=>
  string(11) "/test/alpha"
  [2]=>
  string(11) "/test/omega"
}
*/

?>
