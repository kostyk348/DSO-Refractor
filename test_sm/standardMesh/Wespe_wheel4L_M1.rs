subshader "Wespe_wheel4L_M1_Material0" "StandardMesh/Default"
{
	lighting true;
	lightingSpecular false;
	materialDiffuse 1 1 1;
	twosided true;
      alphaTestRef 0.7;
	texture "texture/Wespe_Whe3_Z";
}

subshader "Wespe_wheel4L_M1_Material1" "StandardMesh/Default"
{
	lighting true;
	lightingSpecular false;
	materialDiffuse 1 1 1;
	texture "texture/Wespe_Whe3_Z";
}

subshader "Wespe_wheel4L_M1_Material2" "StandardMesh/Default"
{
	lighting true;
	lightingSpecular true;
	materialDiffuse 1 1 1;
	materialSpecular 0.101961 0.101961 0.101961;
	materialSpecularPower 12.5;
	texture "texture/tankhatch_h";
}

