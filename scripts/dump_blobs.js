files = util.findFiles('/Users/fcole/Projects/shapestats/models/', '*.obj');

o_path = '/Users/fcole/Projects/shapestats/data/blobs2/';

mainwindow.fitViewerSize('400x400');

for (m in files) {
  model = files[m];

  mainwindow.openScene(model);

  m_name = util.basename(model);
  if (m_name.match('\w*_n$')) {
    m_name = m_name.substr(0, m_name.length-2);
  }

  for (var i = 1; i <= 3; i++) {
      viewer.setRandomCamera(i);
      
      lines_name = o_path + m_name + '_' + i + '_l.png';
      normals_name = o_path + m_name + '_' + i + '_n.pfm';
      depth_name = o_path + m_name + '_' + i + '_d.pfm';

      lines_enable_lines.setValue(true);
      style_background.setValue("White");
      style_mesh_color.setValue("White");
      mainwindow.saveScreenshot(lines_name);

      lines_enable_lines.setValue(false);
      style_background.setValue("Black");
      style_mesh_color.setValue("Normals");
      mainwindow.saveScreenshot(normals_name);

      style_mesh_color.setValue("Depth");
      mainwindow.saveScreenshot(depth_name);
  }
}
