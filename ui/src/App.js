import './App.css';

import Sources from './components/Sources/Sources';
import Graph from './components/Graph/Graph';

function App() {
  const sources = ["L1", "Wohnzimmer"];
  return (
    <div className="App">
      <Sources sources={sources} />
      <Graph />
    </div>
  );
}

export default App;
