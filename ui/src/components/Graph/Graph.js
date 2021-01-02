import React from 'react';

import './Graph.css'

import { ResponsiveContainer, LineChart, Line, CartesianGrid, XAxis, YAxis, Tooltip } from 'recharts';

const graph = (props) => {

    const data = [
        { time: '0', L0: 100, Wohnzimmer: 20 },
        { time: '1', L0: 50, Wohnzimmer: 10 },
        { time: '2', L0: 10, Wohnzimmer: 30 }
    ];

    return (
        <div className="Graph">
            <ResponsiveContainer width='100%' aspect={16.0 / 9.0}>
                <LineChart
                    data={data}
                    margin={{ top: 5, right: 20, bottom: 5, left: 0 }}
                >
                    <Line type="monotone" dataKey="L0" stroke="#8884d8" />
                    <CartesianGrid stroke="#ccc" />
                    <XAxis dataKey="time" />
                    <YAxis />
                    <Tooltip />
                </LineChart>
            </ResponsiveContainer>
        </div>
    );
};

export default graph;