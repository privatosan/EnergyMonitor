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
      data: new Map(),
      graphData: []
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
        const updatedData = new Map(this.state.data);
        updatedData.set(dataFromServer.data.name, dataFromServer.data.values);
        // build a map with entries for each time step
        const timeStepMap = new Map();
        updatedData.forEach((channelData, channelName) => {
          channelData.forEach((element) => {
            // convert from us to periods
            const time = element[0] / 20000.0;
            const timeStep = timeStepMap.get(time);
            if (timeStep) {
              timeStep[channelName] = element[1];
            }
            else {
              timeStepMap.set(time, { [channelName]: element[1] });
            }
          })
        });
        // now build the final graph data with the format
        // [
        //  { time: '0', L0: 100, Wohnzimmer: 20 },
        //  { time: '1', L0: 50, Wohnzimmer: 10 },
        //  { time: '2', L0: 10, Wohnzimmer: 30 }
        // ];
        const graphData = [];
        timeStepMap.forEach((data, time) => {
          graphData.push({ time: time, ...data });
        });
        graphData.sort((a, b) => (a.time - b.time));
        this.setState({ data: updatedData, graphData: graphData });
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
        <Graph data={this.state.graphData} dataKeys={[...this.state.selected]} />
      </div>
    );
  }
}

export default App;
