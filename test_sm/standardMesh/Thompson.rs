subshader "Thompson_Material0" "StandardMesh/Default"
{
	lighting true;
	lightingSpecular true;
	materialDiffuse 0.5 0.5 0.5;
	materialSpecular 0.9 0.9 0.9;
	materialSpecularPower 12.5;
	twosided true;
	texture "texture/";
}

subshader "Thompson_Material1" "StandardMesh/Default"
{
	lighting true;
	lightingSpecular true;
	materialDiffuse 1 1 1;
	materialSpecular 0.0784314 0.0784314 0.0784314;
	materialSpecularPower 12.5;
	envmap true;
	texture "texture/Thom_o";
}

subshader "Thompson_Material2" "StandardMesh/Default"
{
	lighting true;
	lightingSpecular true;
	materialDiffuse 1 1 1;
	materialSpecular 0.392157 0.392157 0.392157;
	materialSpecularPower 12.5;
	envmap true;
	texture "texture/Thom_o";
}

subshader "Thompson_Material3" "StandardMesh/Default"
{
	lighting true;
	lightingSpecular true;
	materialDiffuse 1 1 1;
	materialSpecular 1 1 1;
	materialSpecularPower 12.5;
	envmap true;
	texture "texture/Thom_o";
}

