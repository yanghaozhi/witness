<?php

class UserDao
{
	public function getDetailByUid($userId)
	{
		$data = array(
			"1"=>array(
				"username"=>"小明",
				"password"=>"123456",
				"age"=>30,
				"createTime"=>"1234567890",			
			),	
			"2"=>array(
				"username"=>"小陈",
				"password"=>"234555",
				"age"=>23,
				"createTime"=>"1399999999",			
			),
		);
		return $data[$userId];
	}
}