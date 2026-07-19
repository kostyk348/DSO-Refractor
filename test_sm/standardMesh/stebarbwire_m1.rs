subshader "stebarbwire_m1_Material0" "StandardMesh/Default"
{
	lighting true;
	lightingSpecular true;
	materialDiffuse 0.988235 0.980392 0.980392;
	materialSpecular 0 0 0;
	materialSpecularPower 12.5;
	transparent true;
	twosided true;
	depthWrite false;
	alphaTestRef 0.7;
	texture "texture/barbwr_s";
}

subshader "stebarbwire_m1_Material1" "StandardMesh/Default"
{
	lighting true;
	lightingSpecular true;
	materialDiffuse 1 1 1;
	materialSpecular 0 0 0;
	materialSpecularPower 12.5;
	texture "texture/barbwd_s";
}

