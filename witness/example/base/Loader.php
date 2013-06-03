<?php

class Loader 
{ 
	public static function autoload($name) 
	{ 
		$includeDir =array(
			"base",
			"controller",
			"services",
			"dao"
		);
		foreach($includeDir as $dir)
		{
			$path = "./".$dir."/".$name .'.php';
			if(is_file($path))
			{
				include_once($path);
				return true;
			}
		}
		 
	} 
} 
spl_autoload_register(array('Loader', 'autoload')); 

