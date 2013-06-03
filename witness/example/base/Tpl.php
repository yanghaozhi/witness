<?php
class Tpl
{
	private $tplDir = "./tpl/";

	public function render($tplFilename, $param=array())
	{
		extract($param, EXTR_OVERWRITE);
		$tplFilename = $this->tplDir . $tplFilename.".html";
		ob_start();
		include $tplFilename;
		$content = ob_get_contents();
		ob_end_clean();
		return $content;
	}
}


