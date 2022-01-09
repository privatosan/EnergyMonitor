import React from 'react';

import './Sources.css';

import Source from './Source/Source';

const sources = (props) => {
    const sourceList = props.sources.map(source => {
        return <Source name={source} key={source} />;
    });

    return (
        <div className="Sources">
            {sourceList}
        </div>
    );
};

export default sources;