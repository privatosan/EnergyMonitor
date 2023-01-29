import React, { Component } from 'react';

import './App.css';

import Sources from './components/Sources/Sources';
import Graph from './components/Graph/Graph';

import { w3cwebsocket as W3CWebSocket } from "websocket";

export const updateObject = (oldObject, updatedProperties) => {
  return {
    ...oldObject,
    ...updatedProperties
  }
}

class App extends Component {
  constructor(props) {
    super(props)

    this.state = {
      client: new W3CWebSocket('ws://192.168.178.21:9002'),
      names: ["L1", "Wohnzimmer"],
      selected: new Set(),
      data: new Map()
    }

    this.state.client.onopen = () => {
      console.log('WebSocket Client Connected');
      this.state.client.send(JSON.stringify({
        type: "GetNames"
      }));
    };
    this.state.client.onmessage = (message) => {
      const dataFromServer = JSON.parse(message.data);
      if (dataFromServer.type === "Names") {
        this.setState({ names: Object.values(dataFromServer.data.names) });
      }
      if (dataFromServer.type === "Values") {
        // build the final graph data with the format
        // { "L0":
        //  [
        //   { "x": 0, "y": 100 },
        //   { "x": 1, "y": 50 },
        //   { "x": 2, "y": 10 }
        //  ]
        // },
        // { "Wohnzimmer":
        //  [
        //   { "x": 0, y": 20 },
        //   { "x": 1, y": 10 },
        //   { "x": 2, y": 30 }
        //  ]
        // };
        const unit = dataFromServer.data.name.endsWith("_voltage") ? "voltage" : "current";
        const graphData = dataFromServer.data.values.map(element => (
          { "time": element[0] / 20000.0, [unit]: element[1] }
        ))
        const updatedData = new Map(this.state.data);
        updatedData.set(dataFromServer.data.name, graphData);
        this.setState({ data: updatedData });
      }
    };
  }

  sourceSelectionChanged = (name, value) => {
    const updatedSelected = new Set(this.state.selected);
    if (value) {
      updatedSelected.add(name);
      this.state.client.send(JSON.stringify({
        type: "GetValues",
        data: { name: name }
      }));
    }
    else {
      updatedSelected.delete(name);
      const updatedData = new Map(this.state.data);
      updatedData.delete(name);
      this.setState({
        data: updatedData
      });
    }
    this.setState({
      selected: updatedSelected
    });
  }

  render() {
    return (
      <div className="App">
        <Sources sources={this.state.names} onChange={(name, value) => this.sourceSelectionChanged(name, value)} />
        <Graph data={this.state.data} />
      </div>
    );
  }
}

export default App;
