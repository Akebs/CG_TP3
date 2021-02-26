#### Akebs@Computação Gráfica: TP3 - Jogo/Animação @2020/05
Transformações particulares, efeitos físicos e a inclusão de modelização de objetos não rígidos, quer com vértices modulados explicitamente, quer com vértices (com massa) ligados por molas e amortizadores


##### Actualização sobre o anterior trabalho

- Implementação de uma estrutura de vértices com oscilações harmónicas amortecidas
- Inclusão de efeitos de refração e reflexão do ambiente na água
- Introdução de uma caixa de ambiente (“skybox”)
- Correção do modo de anti-aliasing, de iluminação, de sombreamento, e de texturas
- Melhorias em mostradores e menu

#### Descrição

##### Oscilações harmónicas amortecidas

Implementou-se uma estrutura composta por vértices ligados por molas, onde, através de oscilações harmónicas amortecidas, se pretende representar um líquido em movimento.

<img src="https://github.com/Akebs/CG_TP3/blob/master/CG_TP3/assets/springvertices.png" height="100"/>

Por forma a evitar torções entre vértices foram também incluídas molas diagonais

Através da equação de movimento, obtém-se a aceleração, e, por conseguinte, a velocidade e posição de um objeto. A fórmula para uma oscilação harmónica com amortecimento é a seguinte:^ [7]


<img src="https://github.com/Akebs/CG_TP3/blob/master/CG_TP3/assets/f1.png" height="30"/>

, a qual pode ser escrita como [7]

<img src="https://github.com/Akebs/CG_TP3/blob/master/CG_TP3/assets/f2.png" height="50"/>

, onde k é a constante da elasticidade da mola, x é o vector que liga os dois extremos da mola, |x| é o comprimento de x, d é a distância em repouso da mola, b é a contante do amortecimento da mola e v é a velocidade relativa entre as extremidades da mola.

Além das forças resultantes das molas presentes, são também consideradas, a atração gravítica, e por oposição, a reação à mesma. A força de reação tem também um comportamento oscilatório harmónico amortecido, com profundidade zero tomada como o ponto de equilíbrio.

##### Algoritmos implementados
```
  // Compute gravitational force
  if (GRAVITY > 0.0f){
    for (r = 0 ; r <= MAP_SIZE_ZZ; ++r) {
      for (c = 0 ; c <= MAP_SIZE_XX; ++c) {
        Particle & p1 =mesh[r][c];
        if (p1.position.Y > 0.0f) {
        p1.forces.Y += - GRAVITY;
        }
      }
    }
  }
```

```
  // Compute ground reaction forces
  for (r = 0 ; r <= MAP_SIZE_ZZ; ++r) {
  for (c = 0 ; c <= MAP_SIZE_XX; ++c) {
    Particle & p1 = mesh[r][c];
    ground = p1;
    ground.position.Y = - WATER_DEPTH;
    deltaX = p1.position - ground.position;
    deltaV = p1.velocity;
    float force = (
      GROUND_TENSION * (deltaX.length() - WATER_DEPTH) +
      GROUND_DAMPING * (math:: DotV3 ( deltaV, deltaX ) / deltaX.length())
    );
    f1 = deltaX.normalized() * - force ;
    p1.forces.Y += f1.Y;
  }
}
```

```
// Compute mesh spring forces
int i;
for ( i = 0 ; i < numSprings; ++i){
  Spring & s = cloth[i];
  Particle & p1 = s.p1;
  Particle & p2 = s.p2;
  deltaX = p1.position - p2.position;
  deltaV = p1.velocity - p2.velocity;
  float force = (
    s.tension * (deltaX.length() - s.restLength) +
    s.damping * (math:: DotV3 ( deltaV, deltaX ) / deltaX.length())
  );
  f1 = - deltaX.normalized() * force;
  f2 = - f1;
  p1.forces.Y += f1.Y;
  p2.forces.Y += f2.Y;
}
```


<img src="https://github.com/Akebs/CG_TP3/blob/master/CG_TP3/assets/png1.png" width="720"/>

<img src="https://github.com/Akebs/CG_TP3/blob/master/CG_TP3/assets/png3.png" width="720"/>


##### Efeitos de refração e reflexão do ambiente na água

Por forma a retratar o efeito de reflexão e refração numa superfície não opaca introduziram-se duas texturas. Uma textura corresponde ao fundo e outra imagem corresponde à imagem reflectida nessa superfície.
Para encontrar as coordenadas da imagem refractada é utilizado o método de Gustavo Oliveira[^8 ].


Evita-se a utilização das relações trigonométricas da lei de Snell , 

<img src="https://github.com/Akebs/CG_TP3/blob/master/CG_TP3/assets/f7.png" height="30"/>

Estas são substituídas por um coeficiente de refração, aumentando ou diminuindo a contribuição do raio reflectido na componente final da luz incidente

<img src="https://github.com/Akebs/CG_TP3/blob/master/CG_TP3/assets/f3.png" height="30"/>

A soma vectorial é apresentada na figura 2.

<img src="https://github.com/Akebs/CG_TP3/blob/master/CG_TP3/assets/fig3.png" height="400"/>

A projeção desse vector no “fundo” do liquido terá componente yy,

<img src="https://github.com/Akebs/CG_TP3/blob/master/CG_TP3/assets/f4.png" height="30"/>

Tendo a profundidade p é dada pela equação

<img src="https://github.com/Akebs/CG_TP3/blob/master/CG_TP3/assets/f5.png" height="25"/>

, onde pi é a profundidade em repouso, yi é a componente resultante da deslocação vertical da superfície.

Resulta então

<img src="https://github.com/Akebs/CG_TP3/blob/master/CG_TP3/assets/f6.png" height="70"/>

Já podemos então procurar as componentes x e z do vector de refração.

<img src="https://github.com/Akebs/CG_TP3/blob/master/CG_TP3/assets/fig3.png" height="400"/>

A normalização das coordenadas UV da textura é feita por interpolação, considerando como magnitude o tamanho do mapa de coordenadas, ou seja, do líquido.

##### Algoritmo implementado
```
// Compute texture coordinates
void genTexCoordinates () {
int r, c;
Vector3 incident;
Vector3 refracted;
for (r = 0 ; r <= MAP_SIZE_ZZ ; ++r) {
  for (c = 0 ; c <= MAP_SIZE_XX; ++c) {
    Particle & p = mesh[r][c];
    incident = (p.position - camera.Position).normalized();
    refracted = (-p.normal * REFRACTION_FACTOR + incident).normalized();
    float depth = WATER_DEPTH + p.position.Y;
    float refractionCoef = depth / - refracted.Y;
    p.texCoords.X = (p.position.X + refracted.X * refractionCoef) / MAP_SIZE_XX;
    p.texCoords.Y = (p.position.Z + refracted.Z * refractionCoef) / MAP_SIZE_ZZ;
    }
  }
}
```

Para a renderização recorreu-se à biblioteca Glew para permitir o multitextering.

Usaram-se as seguintes propriedades de texturização
```
void drawWaterText () {
  // Refracted ground
  if (glActiveTexture==NULL)
    cout << "glActiveTexture not supported"<< endl ;
  glActiveTexture( GL_TEXTURE0 );
  glEnable ( GL_TEXTURE_2D );
  glEnable (GL_BLEND);
  glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  // Underwater mapped texture
  glBindTexture ( GL_TEXTURE_2D, SEA_SAND );
  GLfloat env_color[ 4 ] = { 0.2f, 0.2f, 0.4f, 0.4f }; // 30% grey 30% transparency
  glTexEnvfv ( GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, env_color );
  // Sets the wrap parameter for texture coordinates to be mirror repeated
  glTexParameteri (GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_MIRRORED_REPEAT);
  glTexParameteri (GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_MIRRORED_REPEAT);
  glTexEnvf ( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE );
  glTexEnvf ( GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_INTERPOLATE );
  glDisable ( GL_TEXTURE_GEN_S );
  glDisable ( GL_TEXTURE_GEN_T );
  // Environment mapped sky
  glActiveTexture( GL_TEXTURE1 );
  glEnable ( GL_TEXTURE_2D );
  glBindTexture ( GL_TEXTURE_2D, SCATTERED_SKY );
  glTexGeni ( GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP );
  glTexGeni ( GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP );
  glEnable ( GL_TEXTURE_GEN_S );
  glEnable ( GL_TEXTURE_GEN_T );
  // draw polygonized mesh
  drawWaterPoly ();
  glActiveTexture( GL_TEXTURE0 );
  glDisable ( GL_TEXTURE_2D );
  glActiveTexture( GL_TEXTURE1 );
  glDisable ( GL_TEXTURE_2D );
  glBindTexture (GL_TEXTURE_2D, 0 );
  glDisable (GL_BLEND);
}
```

##### Introdução de uma caixa de ambiente (“skybox”) e de uma esfera representando o sol

<img src="https://github.com/Akebs/CG_TP3/blob/master/CG_TP3/assets/png6.png" width="720"/>

<img src="https://github.com/Akebs/CG_TP3/blob/master/CG_TP3/assets/skybox.png" width="720"/>

<img src="https://github.com/Akebs/CG_TP3/blob/master/CG_TP3/assets/png5.png" width="720"/>

##### Implementação de um sistema de lógica de jogo e deteção de colisões entre os elementos dinâmicos

Para diversificar as opções de jogabilidade foram introduzidos temporizadores e mecanismos de deteção de colisão entre o barco e os objectos que possam determinar o resultado da pontuação.
<img src="https://github.com/Akebs/CG_TP3/blob/master/CG_TP3/assets/png4.png" width="720"/>

<img src="https://github.com/Akebs/CG_TP3/blob/master/CG_TP3/assets/png9.png" width="720"/>

Dada a movimentação ser bidimensional, a detecção é feita por aferição da diferença entre distâncias entre os objectos e os raios de contenção atribuídos a cada um.

<img src="https://github.com/Akebs/CG_TP3/blob/master/CG_TP3/assets/fig4.png" height="150"/>

##### Introdução de uma classe para a camara

 O sistema de posicionamento da câmara foi revisto por forma a permitir a sua localização em função dos eixos transversal e longitudinal e respectivos ângulos de Euler[9].

```
// Calculates the front vector from camera Euler Angles (yaw,pitch)
void updateCameraVectors () {
  // Calculate the new Front vector
  Vector3 front;
  front.X = cos (DEG_TO_RADIANS * Yaw) * cos (DEG_TO_RADIANS * Pitch);
  front.Y = sin (DEG_TO_RADIANS * Pitch);
  front.Z = sin (DEG_TO_RADIANS * Yaw) * cos (DEG_TO_RADIANS * Pitch);
  Front = front.normalized();
  float posX = DISTANCE * sin (Yaw * DEG_TO_RADIANS);
  float posY = DISTANCE * sin (Pitch * DEG_TO_RADIANS);
  float posZ = DISTANCE * cos (Yaw * DEG_TO_RADIANS);
  Up = Vector3 (0.0f, 1.0f, 0.0f);
  Position = Vector3 (posX,posY,posZ);
}
```

<img src="https://github.com/Akebs/CG_TP3/blob/master/CG_TP3/assets/png2.png" width="720"/>


##### Video
<a href="https://youtu.be/UeuFzo4uCzs"><img src="https://github.com/Akebs/CG_TP3/blob/master/CG_TP3/assets/canvas.png" width="480" ></a>


##### Observações
 A implementação conseguida ficou aquém do pretendido.
 A dificuldade em afinar o comportamento do sistema de amortecimento não permitiu garantir as movimentações horizontais dos vértices.
 A morosidade no ajuste e previsualização dos parâmetros de texturização, de propriedades dos materiais e de colorização não conduziu a um melhor resultado gráfico.


##### Ambiente de execução e compilação:
- G++
- OpenGL 4.
- Freeglut 3.2.
- GLEW 2.1.


##### Referências
1. Introdução à OpenGL, Prof. Isabel Harb Manssour, https://www.inf.pucrs.br/~manssour/OpenGL/Iluminacao.html, acedido a 18 de maio de 2020

2. OpenGLUT API Reference, http://openglut.sourceforge.net/group__api.html, acedido a 18 de maio de 2020

3. The freeglut Project :: API Documentation , http://freeglut.sourceforge.net/docs/api.php, acedido a 18 de maio de 2020

4. OpenGL Programming Guide - The Official Guide to Learning OpenGL, Version 1. https://www.glprogramming.com/red/chapter01.html, acedido a 18 de maio de 2020
5. Joey de Vries- Getting-started, Coordinate-Systems , https://learnopengl.com/Getting-started/Coordinate-Systems, acedido a 18 de maio de 2020
6. Song Ho Ahn , OpenGL Transformation, https://www.songho.ca/opengl/gl_transform.html, acedido a 18 de maio de 2020

7. Glenn Fiedler, “Spring Physics”, https://gafferongames.com/post/spring_physics, acedido a 18 de maio de 2020

8. Gustavo Oliveira, Refractive Texture Mapping, Part Two https://www.gamasutra.com/view/feature/131531/refractive_texture_mapping_part_.php, , acedido a 18 de maio de 2020

9. Joey de Vries, Camera, https://learnopengl.com/Getting-started/Camera, acedido a 18 de maio de 2020




##### Power_boat.obj  
cedido por GrabCad Community, https://grabcad.com/library/pafu-power-boat-9-5m-1



##### Compilar
```
g++ -O0 -g3 -Wall -c -fmessage-length=0 -o "src\\CG_TP3.o" "..\\src\\CG_TP3.cpp"
g++ -pthread -o CG_TP3 "src\\CG_TP3.o" -lglew32 -lopengl32 -lfreeglut -lglu
```



